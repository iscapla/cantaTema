/**
 * @file faiss_similarity_search.cpp
 * @brief Implements the FaissSimilaritySearch class.
 */

#include "similarity/faiss_similarity_search.hpp"
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
            res.similarity_score = 0.0f;
            res.is_mentioned = false;
            
            float weight = (i < importance_weights.size()) ? importance_weights[i] : 1.0f;
            res.weighted_missed_score = weight;
            results.push_back(res);
        }
        return results;
    }

    for (size_t i = 0; i < pdf_embeddings.size(); ++i) {
        const auto& pdf_vec = pdf_embeddings[i];
        float weight = (i < importance_weights.size()) ? importance_weights[i] : 1.0f;

        SimilarityResult res;
        res.pdf_chunk_index = i;
        res.best_transcript_chunk_index = -1;
        res.similarity_score = 0.0f;
        res.is_mentioned = false;
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

        float best_score = -2.0f; // Cosine similarity ranges from -1 to 1
        int best_idx = -1;

        for (size_t j = 0; j < m_indexed_embeddings.size(); ++j) {
            const auto& trans_vec = m_indexed_embeddings[j];

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

            float score = static_cast<float>(dot_product / (pdf_norm * trans_norm));
            if (score > best_score) {
                best_score = score;
                best_idx = static_cast<int>(j);
            }
        }

        if (best_idx != -1) {
            // Clamp score to [-1.0, 1.0] range
            best_score = std::max(-1.0f, std::min(1.0f, best_score));
            
            res.best_transcript_chunk_index = best_idx;
            res.similarity_score = best_score;
            res.is_mentioned = (best_score >= similarity_threshold);
            
            if (res.is_mentioned) {
                res.weighted_missed_score = 0.0f;
            } else {
                res.weighted_missed_score = weight * (1.0f - std::max(0.0f, best_score));
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
