/**
 * @file hybrid_paraphrase_matcher.cpp
 * @brief Implementation of HybridParaphraseMatcher combining Tier 1 dictionary and Tier 2 neural embedding matching.
 */

#include "similarity/hybrid_paraphrase_matcher.hpp"

HybridParaphraseMatcher::HybridParaphraseMatcher(
    std::shared_ptr<IEmbeddingEngine> embedding_engine,
    float embedding_threshold
)
    : m_dict_matcher(),
      m_embed_matcher(std::move(embedding_engine), embedding_threshold)
{
}

bool HybridParaphraseMatcher::is_synonym(const std::string& word1, const std::string& word2, const std::string& language) const {
    // 1. Tier 1 Fast Dictionary lookup
    if (m_dict_matcher.is_synonym(word1, word2, language)) {
        return true;
    }

    // 2. Tier 2 Neural Embedding fallback
    return m_embed_matcher.is_synonym(word1, word2, language);
}

std::vector<std::string> HybridParaphraseMatcher::get_synonyms(const std::string& word, const std::string& language) const {
    return m_dict_matcher.get_synonyms(word, language);
}

ParaphraseMatchResult HybridParaphraseMatcher::compare_phrases(
    const std::string& ref_phrase,
    const std::string& trans_phrase,
    const std::string& domain_key,
    const std::string& language
) const {
    // 1. Tier 1 Fast Dictionary / Domain rule lookup
    auto dict_res = m_dict_matcher.compare_phrases(ref_phrase, trans_phrase, domain_key, language);
    if (dict_res.is_match) {
        return dict_res;
    }

    // 2. Tier 2 Neural Embedding fallback
    return m_embed_matcher.compare_phrases(ref_phrase, trans_phrase, domain_key, language);
}

std::vector<ParaphraseMatchResult> HybridParaphraseMatcher::find_paraphrases(
    const std::vector<std::string>& ref_words,
    const std::vector<std::string>& trans_words,
    const std::string& domain_key,
    const std::string& language
) const {
    if (ref_words.empty() || trans_words.empty()) {
        return {};
    }

    // 1. Run Tier 1 Dictionary search
    auto dict_matches = m_dict_matcher.find_paraphrases(ref_words, trans_words, domain_key, language);

    std::vector<bool> ref_used(ref_words.size(), false);
    std::vector<bool> trans_used(trans_words.size(), false);

    for (const auto& m : dict_matches) {
        for (size_t k = 0; k < m.ref_word_count && (m.ref_start_index + k < ref_words.size()); ++k) {
            ref_used[m.ref_start_index + k] = true;
        }
        for (size_t k = 0; k < m.trans_word_count && (m.trans_start_index + k < trans_words.size()); ++k) {
            trans_used[m.trans_start_index + k] = true;
        }
    }

    // 2. Run Tier 2 Neural search on remaining unconsumed tokens
    std::vector<ParaphraseMatchResult> all_matches = dict_matches;

    // Check remaining word spans
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

                    auto match = m_embed_matcher.compare_phrases(r_str, t_str, domain_key, language);
                    if (match.is_match) {
                        match.ref_start_index = ri;
                        match.ref_word_count = r_len;
                        match.trans_start_index = ti;
                        match.trans_word_count = t_len;
                        match.is_multi_word_phrase = (r_len > 1 || t_len > 1);

                        for (size_t k = 0; k < r_len; ++k) ref_used[ri + k] = true;
                        for (size_t k = 0; k < t_len; ++k) trans_used[ti + k] = true;

                        all_matches.push_back(match);
                        break;
                    }
                }
            }
        }
    }

    return all_matches;
}
