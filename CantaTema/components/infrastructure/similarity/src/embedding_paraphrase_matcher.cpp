/**
 * @file embedding_paraphrase_matcher.cpp
 * @brief Implementation of EmbeddingParaphraseMatcher for Tier 2 neural semantic paraphrase matching.
 */

#include "similarity/embedding_paraphrase_matcher.hpp"
#include "similarity/word_sequence_aligner.hpp"
#include "configuration/configuration_system.hpp"
#include <cmath>
#include <cctype>
#include <algorithm>

EmbeddingParaphraseMatcher::EmbeddingParaphraseMatcher(
    std::shared_ptr<IEmbeddingEngine> engine,
    float similarity_threshold
)
    : m_engine(std::move(engine)),
      m_threshold(similarity_threshold)
{
    if (m_threshold <= 0.0f) {
        m_threshold = ConfigurationSystem::getInstance().get_semantic_paraphrase_embedding_threshold();
    }
}

float EmbeddingParaphraseMatcher::compute_cosine_similarity(const std::vector<float>& vec1, const std::vector<float>& vec2) {
    if (vec1.empty() || vec2.empty() || vec1.size() != vec2.size()) {
        return 0.0f;
    }

    double dot = 0.0;
    double norm1 = 0.0;
    double norm2 = 0.0;

    for (size_t i = 0; i < vec1.size(); ++i) {
        dot += static_cast<double>(vec1[i]) * static_cast<double>(vec2[i]);
        norm1 += static_cast<double>(vec1[i]) * static_cast<double>(vec1[i]);
        norm2 += static_cast<double>(vec2[i]) * static_cast<double>(vec2[i]);
    }

    if (norm1 <= 1e-9 || norm2 <= 1e-9) {
        return 0.0f;
    }

    float sim = static_cast<float>(dot / (std::sqrt(norm1) * std::sqrt(norm2)));
    return std::max(-1.0f, std::min(1.0f, sim));
}

bool EmbeddingParaphraseMatcher::passes_safety_guards(const std::string& ref_span, const std::string& trans_span) {
    // 1. Numeric and digit safety check: numbers must match exactly if present
    std::string ref_digits, trans_digits;
    for (char c : ref_span) {
        if (std::isdigit(static_cast<unsigned char>(c))) ref_digits += c;
    }
    for (char c : trans_span) {
        if (std::isdigit(static_cast<unsigned char>(c))) trans_digits += c;
    }
    if (ref_digits != trans_digits) {
        return false; // Numbers or dates differ!
    }

    // 2. Negation safety check
    auto check_negation = [](const std::string& s) {
        std::string norm = WordSequenceAligner::normalize_word(s);
        auto tokens = WordSequenceAligner::tokenize(norm);
        for (const auto& t : tokens) {
            if (t == "no" || t == "not" || t == "nunca" || t == "jamas" || t == "tampoco" || t == "never") {
                return true;
            }
        }
        return false;
    };

    if (check_negation(ref_span) != check_negation(trans_span)) {
        return false; // One span is negated while the other is affirmative!
    }

    return true;
}

bool EmbeddingParaphraseMatcher::is_synonym(const std::string& word1, const std::string& word2, const std::string& language) const {
    (void)language;
    auto res = compare_phrases(word1, word2);
    return res.is_match;
}

std::vector<std::string> EmbeddingParaphraseMatcher::get_synonyms(const std::string& word, const std::string& language) const {
    (void)word;
    (void)language;
    // Embedding matcher is dynamic, does not maintain a fixed reverse lookup list
    return {};
}

ParaphraseMatchResult EmbeddingParaphraseMatcher::compare_phrases(
    const std::string& ref_phrase,
    const std::string& trans_phrase,
    const std::string& domain_key,
    const std::string& language
) const {
    (void)domain_key;
    (void)language;
    ParaphraseMatchResult result;

    std::string r_norm = WordSequenceAligner::normalize_word(ref_phrase);
    std::string t_norm = WordSequenceAligner::normalize_word(trans_phrase);

    if (r_norm.empty() || t_norm.empty()) {
        return result;
    }

    auto r_tokens = WordSequenceAligner::tokenize(r_norm);
    auto t_tokens = WordSequenceAligner::tokenize(t_norm);

    if (r_norm == t_norm) {
        result.is_match = true;
        result.similarity_score = 1.0f;
        result.matched_reference_phrase = ref_phrase;
        result.matched_transcript_phrase = trans_phrase;
        result.ref_word_count = r_tokens.size();
        result.trans_word_count = t_tokens.size();
        result.is_multi_word_phrase = false;
        return result;
    }

    if (!passes_safety_guards(ref_phrase, trans_phrase)) {
        return result; // Disqualified by domain safety rules
    }

    if (!m_engine) {
        return result;
    }

    auto v1 = m_engine->generate_embedding(ref_phrase, EmbeddingRole::DEFAULT);
    auto v2 = m_engine->generate_embedding(trans_phrase, EmbeddingRole::DEFAULT);

    if (v1.empty() || v2.empty()) {
        return result;
    }

    float sim = compute_cosine_similarity(v1, v2);
    if (sim >= m_threshold) {
        result.is_match = true;
        result.similarity_score = sim;
        result.matched_reference_phrase = ref_phrase;
        result.matched_transcript_phrase = trans_phrase;
        result.ref_word_count = r_tokens.size();
        result.trans_word_count = t_tokens.size();
        result.is_multi_word_phrase = (r_tokens.size() > 1 || t_tokens.size() > 1);
    }

    return result;
}

std::vector<ParaphraseMatchResult> EmbeddingParaphraseMatcher::find_paraphrases(
    const std::vector<std::string>& ref_words,
    const std::vector<std::string>& trans_words,
    const std::string& domain_key,
    const std::string& language
) const {
    std::vector<ParaphraseMatchResult> results;
    if (!m_engine || ref_words.empty() || trans_words.empty()) return results;

    std::vector<bool> ref_used(ref_words.size(), false);
    std::vector<bool> trans_used(trans_words.size(), false);

    // Sliding window: test 1 to 4 words
    for (size_t r_len = 4; r_len >= 1; --r_len) {
        for (size_t ri = 0; ri + r_len <= ref_words.size(); ++ri) {
            bool ref_free = true;
            for (size_t k = 0; k < r_len; ++k) {
                if (ref_used[ri + k]) { ref_free = false; break; }
            }
            if (!ref_free) continue;

            std::string r_str;
            for (size_t k = 0; k < r_len; ++k) {
                if (k > 0) r_str += " ";
                r_str += ref_words[ri + k];
            }

            for (size_t t_len = 4; t_len >= 1; --t_len) {
                for (size_t ti = 0; ti + t_len <= trans_words.size(); ++ti) {
                    bool trans_free = true;
                    for (size_t k = 0; k < t_len; ++k) {
                        if (trans_used[ti + k]) { trans_free = false; break; }
                    }
                    if (!trans_free) continue;

                    std::string t_str;
                    for (size_t k = 0; k < t_len; ++k) {
                        if (k > 0) t_str += " ";
                        t_str += trans_words[ti + k];
                    }

                    if (r_str == t_str) continue; // Exact matches are handled by LCS

                    auto match = compare_phrases(r_str, t_str, domain_key, language);
                    if (match.is_match) {
                        match.ref_start_index = ri;
                        match.ref_word_count = r_len;
                        match.trans_start_index = ti;
                        match.trans_word_count = t_len;
                        match.is_multi_word_phrase = (r_len > 1 || t_len > 1);

                        for (size_t k = 0; k < r_len; ++k) ref_used[ri + k] = true;
                        for (size_t k = 0; k < t_len; ++k) trans_used[ti + k] = true;

                        results.push_back(match);
                        break;
                    }
                }
            }
        }
    }

    return results;
}
