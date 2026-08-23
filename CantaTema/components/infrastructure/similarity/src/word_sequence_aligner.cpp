#include "similarity/word_sequence_aligner.hpp"
#include "similarity/phonetic_matcher_manager.hpp"
#include "similarity/paraphrase_matcher_manager.hpp"
#include "configuration/configuration_system.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

std::string WordSequenceAligner::normalize_word(const std::string& input) {
    std::string result;
    result.reserve(input.length());

    for (size_t i = 0; i < input.length(); ++i) {
        unsigned char c = static_cast<unsigned char>(input[i]);
        
        // Strip common ASCII punctuation
        if (c == '.' || c == ',' || c == ';' || c == ':' || c == '?' || c == '!' ||
            c == '(' || c == ')' || c == '"' || c == '\'' || c == '[' || c == ']' ||
            c == '{' || c == '}' || c == '-' || c == '/' || c == '\\') {
            continue;
        }

        // Strip Spanish non-ASCII punctuation (¿ and ¡ in UTF-8: C2 BF and C2 A1)
        if (c == 0xC2 && i + 1 < input.length()) {
            unsigned char c2 = static_cast<unsigned char>(input[i + 1]);
            if (c2 == 0xBF || c2 == 0xA1) {
                i++;
                continue;
            }
        }

        // Basic accent normalization for Spanish UTF-8 bytes
        if (c == 0xC3 && i + 1 < input.length()) {
            unsigned char c2 = static_cast<unsigned char>(input[i + 1]);
            if (c2 == 0xA1 || c2 == 0x81) { result += 'a'; i++; continue; } // á/Á
            if (c2 == 0xA9 || c2 == 0x89) { result += 'e'; i++; continue; } // é/É
            if (c2 == 0xAD || c2 == 0x8D) { result += 'i'; i++; continue; } // í/Í
            if (c2 == 0xB3 || c2 == 0x93) { result += 'o'; i++; continue; } // ó/Ó
            if (c2 == 0xBA || c2 == 0x9A) { result += 'u'; i++; continue; } // ú/Ú
            if (c2 == 0xB1 || c2 == 0x91) { result += 'n'; i++; continue; } // ñ/Ñ
        }

        result += static_cast<char>(std::tolower(c));
    }
    return result;
}

std::vector<std::string> WordSequenceAligner::tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::stringstream ss(text);
    std::string word;
    while (ss >> word) {
        tokens.push_back(word);
    }
    return tokens;
}

WordAlignmentResult WordSequenceAligner::align(
    const std::string& reference_text,
    const std::string& transcript_text,
    const std::string& domain_key,
    const std::string& language
) {
    WordAlignmentResult result;

    std::vector<std::string> ref_orig = tokenize(reference_text);
    std::vector<std::string> trans_orig = tokenize(transcript_text);

    if (ref_orig.empty()) {
        return result;
    }

    result.total_reference_words = ref_orig.size();

    std::vector<std::string> ref_norm;
    ref_norm.reserve(ref_orig.size());
    for (const auto& w : ref_orig) {
        ref_norm.push_back(normalize_word(w));
    }

    std::vector<std::string> trans_norm;
    trans_norm.reserve(trans_orig.size());
    for (const auto& w : trans_orig) {
        trans_norm.push_back(normalize_word(w));
    }

    size_t m = ref_norm.size();
    size_t n = trans_norm.size();

    // Intermediate tracking arrays
    std::vector<bool> ref_matched(m, false);
    std::vector<bool> trans_matched(n, false);
    std::vector<bool> ref_phonetic(m, false);
    std::vector<bool> ref_semantic(m, false);
    std::vector<std::string> ref_semantic_phrase(m);
    std::vector<float> ref_semantic_sim(m, 0.0f);

    // Check if phonetic matching is enabled
    bool enable_phonetic = ConfigurationSystem::getInstance().get_phonetic_enable_matching();
    auto phonetic_matcher = enable_phonetic ? PhoneticMatcherManager::getInstance().get_active_matcher() : nullptr;

    // Check if semantic paraphrasing is enabled
    bool enable_paraphrase = ConfigurationSystem::getInstance().get_semantic_paraphrase_enable();
    float semantic_weight_credit = ConfigurationSystem::getInstance().get_semantic_paraphrase_weight_credit();
    auto paraphrase_matcher = enable_paraphrase ? ParaphraseMatcherManager::getInstance().get_active_matcher() : nullptr;

    std::string resolved_domain = domain_key.empty() ? ConfigurationSystem::getInstance().get_comparison_active_domain() : domain_key;
    std::string resolved_lang = language.empty() ? ConfigurationSystem::getInstance().get_comparison_active_language() : language;

    // Pass 1: Multi-word domain phrase matching (preserving domain concept units)
    if (paraphrase_matcher) {
        auto para_matches = paraphrase_matcher->find_paraphrases(ref_norm, trans_norm, resolved_domain, resolved_lang);
        for (const auto& pm : para_matches) {
            if (!pm.is_match || !pm.is_multi_word_phrase) continue;

            bool ref_free = true;
            for (size_t k = 0; k < pm.ref_word_count; ++k) {
                if (pm.ref_start_index + k >= m || ref_semantic[pm.ref_start_index + k]) { ref_free = false; break; }
            }
            bool trans_free = true;
            for (size_t k = 0; k < pm.trans_word_count; ++k) {
                if (pm.trans_start_index + k >= n || trans_matched[pm.trans_start_index + k]) { trans_free = false; break; }
            }

            if (ref_free && trans_free) {
                for (size_t k = 0; k < pm.ref_word_count; ++k) {
                    size_t r_idx = pm.ref_start_index + k;
                    ref_semantic[r_idx] = true;
                    ref_semantic_phrase[r_idx] = pm.matched_transcript_phrase;
                    ref_semantic_sim[r_idx] = pm.similarity_score;
                }
                for (size_t k = 0; k < pm.trans_word_count; ++k) {
                    trans_matched[pm.trans_start_index + k] = true;
                }
            }
        }
    }

    // Pass 2: DP table for Longest Common Subsequence (LCS) on remaining tokens
    std::vector<std::vector<size_t>> dp(m + 1, std::vector<size_t>(n + 1, 0));

    for (size_t i = 1; i <= m; ++i) {
        for (size_t j = 1; j <= n; ++j) {
            if (!ref_semantic[i - 1] && !trans_matched[j - 1] &&
                !ref_norm[i - 1].empty() && ref_norm[i - 1] == trans_norm[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1] + 1;
            } else {
                dp[i][j] = std::max(dp[i - 1][j], dp[i][j - 1]);
            }
        }
    }

    // Backtrack to identify matched reference indices
    size_t i = m;
    size_t j = n;

    while (i > 0 && j > 0) {
        if (!ref_semantic[i - 1] && !trans_matched[j - 1] &&
            !ref_norm[i - 1].empty() && ref_norm[i - 1] == trans_norm[j - 1]) {
            ref_matched[i - 1] = true;
            trans_matched[j - 1] = true;
            i--;
            j--;
        } else if (dp[i - 1][j] >= dp[i][j - 1]) {
            i--;
        } else {
            j--;
        }
    }

    // Pass 3: Phonetic matching check on remaining unmatched tokens
    if (phonetic_matcher) {
        for (size_t idx = 0; idx < m; ++idx) {
            if (ref_matched[idx] || ref_semantic[idx] || ref_norm[idx].empty()) continue;

            for (size_t tj = 0; tj < n; ++tj) {
                if (!trans_matched[tj] && !trans_norm[tj].empty()) {
                    auto p_res = phonetic_matcher->compare_words(ref_norm[idx], trans_norm[tj]);
                    if (p_res.is_match) {
                        ref_phonetic[idx] = true;
                        trans_matched[tj] = true;
                        break;
                    }
                }
            }
        }
    }

    // Pass 4: Single-word Semantic Synonyms on remaining unmatched tokens
    if (paraphrase_matcher) {
        for (size_t idx = 0; idx < m; ++idx) {
            if (ref_matched[idx] || ref_phonetic[idx] || ref_semantic[idx] || ref_norm[idx].empty()) continue;

            for (size_t tj = 0; tj < n; ++tj) {
                if (!trans_matched[tj] && !trans_norm[tj].empty()) {
                    if (paraphrase_matcher->is_synonym(ref_norm[idx], trans_norm[tj], resolved_lang)) {
                        ref_semantic[idx] = true;
                        ref_semantic_phrase[idx] = trans_orig[tj];
                        ref_semantic_sim[idx] = 0.95f;
                        trans_matched[tj] = true;
                        break;
                    }
                }
            }
        }
    }

    // Token weighting classifier lambda
    auto classify_token = [](const std::string& orig_word, const std::string& norm_word, WordDiffToken& token) {
        // 1. Check if stopword
        static const std::vector<std::string> stopwords = {
            "el", "la", "los", "las", "un", "una", "unos", "unas", "de", "en", "por", "para", "con",
            "del", "al", "su", "sus", "y", "o", "que", "se", "es", "son", "a", "ante", "bajo", "cabe",
            "contra", "desde", "durante", "mediante", "hacia", "hasta", "sin", "sobre", "tras", "e", "u",
            "the", "of", "in", "for", "to", "and", "or", "is", "are", "by", "with", "from", "at"
        };
        if (std::find(stopwords.begin(), stopwords.end(), norm_word) != stopwords.end()) {
            token.is_stopword = true;
            token.weight = 0.2f;
            return;
        }

        // 2. Check if legal reference or enumerator label
        static const std::vector<std::string> legal_terms = {
            "articulo", "art", "ley", "rd", "decreto", "estatuto", "reglamento",
            "titulo", "capitulo", "seccion", "disposicion"
        };
        bool is_legal = (std::find(legal_terms.begin(), legal_terms.end(), norm_word) != legal_terms.end());
        
        // Also check if list enumerator like "a)", "b)", "1º", "2º", "1."
        if (!is_legal && norm_word.length() <= 3 && !norm_word.empty()) {
            bool all_alnum = true;
            for (char c : norm_word) {
                if (!std::isalnum(static_cast<unsigned char>(c))) { all_alnum = false; break; }
            }
            if (all_alnum) {
                is_legal = true;
            }
        }

        if (is_legal) {
            token.is_legal_citation = true;
            token.weight = 4.0f;
            return;
        }

        // 3. Check if numeric or quantitative entity
        bool has_digit = false;
        for (char c : orig_word) {
            if (std::isdigit(static_cast<unsigned char>(c))) { has_digit = true; break; }
        }
        if (has_digit || norm_word.find('%') != std::string::npos || norm_word == "euros" || norm_word == "euro" || norm_word == "porciento") {
            token.is_numeric = true;
            token.weight = 3.0f;
            return;
        }

        // 4. Domain keywords or capitalized proper nouns
        if (!orig_word.empty() && std::isupper(static_cast<unsigned char>(orig_word[0]))) {
            token.weight = 2.0f;
            return;
        }

        // 5. Default content word
        token.weight = 1.0f;
    };

    // Construct final WordDiffToken array
    result.reference_words.reserve(m);
    for (size_t idx = 0; idx < m; ++idx) {
        WordDiffToken token;
        token.original_word = ref_orig[idx];
        token.normalized_word = ref_norm[idx];
        token.index = idx;

        classify_token(token.original_word, token.normalized_word, token);

        if (ref_matched[idx]) {
            token.status = WordDiffStatus::MATCHED;
            result.matched_word_count++;
            result.matched_reference_weight += token.weight;
        } else if (ref_phonetic[idx]) {
            token.status = WordDiffStatus::PHONETIC_MISPRONUNCIATION;
            result.matched_word_count++;
            result.phonetic_word_count++;
            result.matched_reference_weight += (token.weight * 0.85f); // 85% partial credit
        } else if (ref_semantic[idx]) {
            token.status = WordDiffStatus::SEMANTIC_EQUIVALENCE;
            token.equivalent_phrase = ref_semantic_phrase[idx];
            token.semantic_similarity = ref_semantic_sim[idx];
            result.matched_word_count++;
            result.semantic_word_count++;
            result.matched_reference_weight += (token.weight * semantic_weight_credit); // configurable partial credit
        } else {
            token.status = WordDiffStatus::OMITTED;
            result.omitted_word_count++;
            if (token.is_legal_citation) {
                result.has_missing_legal_citation = true;
            }
        }

        result.total_reference_weight += token.weight;
        result.reference_words.push_back(token);
    }

    if (result.total_reference_words > 0) {
        result.word_recall_score = static_cast<float>(result.matched_word_count) / static_cast<float>(result.total_reference_words);
    }
    if (result.total_reference_weight > 0.0f) {
        result.weighted_recall_score = result.matched_reference_weight / result.total_reference_weight;
    }

    return result;
}

