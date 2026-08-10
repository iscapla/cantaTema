#include "similarity/faiss_similarity_search.hpp"
#include "file_handler/numerical_entity_extractor.hpp"
#include "file_handler/lexical_keyword_extractor.hpp"
#include "primitives/utils_logger.hpp"
#include <cmath>
#include <algorithm>

bool FaissSimilaritySearch::index_transcript_embeddings(const std::vector<std::vector<float>>& transcript_embeddings) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (transcript_embeddings.empty()) {
        logger->warn("index_transcript_embeddings called with empty input. Index cleared.");
        m_indexed_embeddings.clear();
        m_dimension = 0;
        return true;
    }

    // Validate dimensions
    size_t dim = transcript_embeddings[0].size();
    if (dim == 0) {
        logger->error("First embedding has size 0. Indexing failed.");
        return false;
    }

    for (size_t i = 1; i < transcript_embeddings.size(); ++i) {
        if (transcript_embeddings[i].size() != dim) {
            logger->error("Embedding at index {} has mismatched dimension (expected {}, got {}). Indexing failed.",
                          i, dim, transcript_embeddings[i].size());
            return false;
        }
    }

    m_indexed_embeddings = transcript_embeddings;
    m_dimension = dim;
    logger->info("Successfully indexed {} transcript embeddings with dimension {}.", m_indexed_embeddings.size(), m_dimension);
    return true;
}

std::vector<SimilarityResult> FaissSimilaritySearch::search_pdf_matches(
    const std::vector<std::vector<float>>& pdf_embeddings,
    const std::vector<float>& importance_weights,
    float similarity_threshold) {

    SimilaritySearchOptions options;
    options.similarity_threshold = similarity_threshold;

    std::vector<std::string> empty_texts;
    return search_pdf_matches_advanced(pdf_embeddings, empty_texts, empty_texts, importance_weights, options);
}

std::vector<SimilarityResult> FaissSimilaritySearch::search_pdf_matches_advanced(
    const std::vector<std::vector<float>>& pdf_embeddings,
    const std::vector<std::string>& pdf_texts,
    const std::vector<std::string>& transcript_texts,
    const std::vector<float>& importance_weights,
    const SimilaritySearchOptions& options) {
    
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<SimilarityResult> results;

    if (pdf_embeddings.empty()) {
        return results;
    }

    results.reserve(pdf_embeddings.size());

    // If there are no indexed transcript embeddings, return all PDF chunks as not mentioned
    if (m_indexed_embeddings.empty()) {
        logger->warn("No transcript embeddings indexed. All search queries marked as not mentioned.");
        for (size_t i = 0; i < pdf_embeddings.size(); ++i) {
            SimilarityResult res;
            res.pdf_chunk_index = i;
            res.best_transcript_chunk_index = -1;
            res.candidate_transcript_chunk_index = -1;
            res.similarity_score = 0.0f;
            res.is_mentioned = false;
            res.has_numeric_warning = false;
            
            float weight = (i < importance_weights.size()) ? importance_weights[i] : 1.0f;
            res.weighted_missed_score = weight;
            results.push_back(res);
        }
        return results;
    }

    int last_matched_j = -1;

    for (size_t i = 0; i < pdf_embeddings.size(); ++i) {
        const auto& pdf_vec = pdf_embeddings[i];
        float weight = (i < importance_weights.size()) ? importance_weights[i] : 1.0f;
        std::string ref_text = (i < pdf_texts.size()) ? pdf_texts[i] : "";

        bool is_list_item = false;
        std::unordered_set<std::string> ref_keywords;
        size_t ref_word_count = 0;

        if (!ref_text.empty()) {
            is_list_item = NumericalEntityExtractor::is_enumerated_item(ref_text);
            ref_keywords = LexicalKeywordExtractor::extract_keywords(ref_text);
            ref_word_count = LexicalKeywordExtractor::count_words(ref_text);
        }

        SimilarityResult res;
        res.pdf_chunk_index = i;
        res.best_transcript_chunk_index = -1;
        res.candidate_transcript_chunk_index = -1;
        res.similarity_score = 0.0f;
        res.is_mentioned = false;
        res.has_numeric_warning = false;
        res.weighted_missed_score = weight; // Default to full weight missed

        if (pdf_vec.size() != m_dimension) {
            logger->error("PDF embedding at index {} has mismatched dimension (expected {}, got {}). Skipping match.",
                          i, m_dimension, pdf_vec.size());
            results.push_back(res);
            continue;
        }

        // Calculate L2 norm of the PDF vector
        double pdf_norm_sq = 0.0;
        for (float val : pdf_vec) {
            pdf_norm_sq += static_cast<double>(val) * val;
        }
        double pdf_norm = std::sqrt(pdf_norm_sq);
        if (pdf_norm < 1e-6) {
            logger->warn("PDF embedding at index {} has near-zero norm. Skipping match.", i);
            results.push_back(res);
            continue;
        }

        float best_effective_score = -2.0f;
        float best_raw_similarity = -2.0f;
        int best_j = -1;
        bool best_has_warning = false;

        for (size_t j = 0; j < m_indexed_embeddings.size(); ++j) {
            const auto& trans_vec = m_indexed_embeddings[j];
            std::string trans_text = (j < transcript_texts.size()) ? transcript_texts[j] : "";

            double dot_product = 0.0;
            double trans_norm_sq = 0.0;
            for (size_t k = 0; k < m_dimension; ++k) {
                dot_product += static_cast<double>(pdf_vec[k]) * trans_vec[k];
                trans_norm_sq += static_cast<double>(trans_vec[k]) * trans_vec[k];
            }

            double trans_norm = std::sqrt(trans_norm_sq);
            if (trans_norm < 1e-6) {
                continue;
            }

            float raw_cosine = static_cast<float>(dot_product / (pdf_norm * trans_norm));
            raw_cosine = std::max(-1.0f, std::min(1.0f, raw_cosine));

            // Hybrid Lexical Gating: for short reference chunks (<= short_chunk_word_threshold), verify keyword overlap
            if (ref_word_count > 0 && ref_word_count <= options.short_chunk_word_threshold && !ref_keywords.empty() && !trans_text.empty()) {
                auto trans_keywords = LexicalKeywordExtractor::extract_keywords(trans_text);
                size_t overlap = LexicalKeywordExtractor::count_keyword_overlap(ref_keywords, trans_keywords);
                if (overlap == 0) {
                    // Zero keyword overlap on a short chunk -> scale raw score down by scaling factor
                    raw_cosine *= options.lexical_mismatch_scaling_factor;
                }
            }

            // Numerical entity analysis
            NumericMatchAnalysis num_analysis;
            if (!ref_text.empty() && !trans_text.empty()) {
                num_analysis = NumericalEntityExtractor::compare_entities(
                    ref_text, trans_text, options.numeric_boost, options.numeric_mismatch_penalty
                );
            }

            float score_adjusted = raw_cosine + num_analysis.score_modifier;

            // Order-aware temporal distance penalty (bidirectional)
            float temporal_penalty = 0.0f;
            if (!is_list_item) {
                // Backward jump penalty
                if (last_matched_j >= 0 && static_cast<int>(j) < last_matched_j) {
                    int dist = last_matched_j - static_cast<int>(j);
                    temporal_penalty += options.temporal_penalty_weight * std::min(1.0f, static_cast<float>(dist) / 10.0f);
                }
                // Unnatural far forward jump penalty relative to estimated position
                if (!pdf_embeddings.empty() && !m_indexed_embeddings.empty()) {
                    int expected_j = static_cast<int>((i * m_indexed_embeddings.size()) / pdf_embeddings.size());
                    if (last_matched_j >= 0) {
                        expected_j = std::max(expected_j, last_matched_j);
                    }
                    if (static_cast<int>(j) > expected_j + 15) {
                        int fwd_dist = static_cast<int>(j) - (expected_j + 15);
                        temporal_penalty += options.temporal_penalty_weight * std::min(1.0f, static_cast<float>(fwd_dist) / 15.0f);
                    }
                }
            }

            float effective_score = score_adjusted - temporal_penalty;

            if (effective_score > best_effective_score) {
                best_effective_score = effective_score;
                best_raw_similarity = raw_cosine;
                best_j = static_cast<int>(j);
                best_has_warning = num_analysis.has_warning;
            }
        }

        if (best_j != -1) {
            res.candidate_transcript_chunk_index = best_j;
            res.similarity_score = std::max(-1.0f, std::min(1.0f, best_effective_score));
            res.is_mentioned = (res.similarity_score >= options.similarity_threshold);
            res.has_numeric_warning = best_has_warning;

            if (best_j >= 0 && best_j < static_cast<int>(transcript_texts.size())) {
                res.word_alignment = WordSequenceAligner::align(ref_text, transcript_texts[best_j]);
                res.word_recall_score = res.word_alignment.word_recall_score;
            }

            if (res.is_mentioned) {
                res.best_transcript_chunk_index = best_j;
                res.weighted_missed_score = 0.0f;
                last_matched_j = best_j;
            } else {
                res.best_transcript_chunk_index = -1; // Unassigned / missing!
                res.weighted_missed_score = weight * (1.0f - std::max(0.0f, res.similarity_score));
            }
        }

        results.push_back(res);
    }

    return results;
}

void FaissSimilaritySearch::reset() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_indexed_embeddings.clear();
    m_dimension = 0;
    logger->info("Similarity index reset.");
}

size_t FaissSimilaritySearch::get_indexed_count() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_indexed_embeddings.size();
}
