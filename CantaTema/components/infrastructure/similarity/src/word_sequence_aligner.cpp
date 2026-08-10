#include "similarity/word_sequence_aligner.hpp"
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

WordAlignmentResult WordSequenceAligner::align(const std::string& reference_text, const std::string& transcript_text) {
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

    // DP table for Longest Common Subsequence (LCS)
    std::vector<std::vector<size_t>> dp(m + 1, std::vector<size_t>(n + 1, 0));

    for (size_t i = 1; i <= m; ++i) {
        for (size_t j = 1; j <= n; ++j) {
            if (!ref_norm[i - 1].empty() && ref_norm[i - 1] == trans_norm[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1] + 1;
            } else {
                dp[i][j] = std::max(dp[i - 1][j], dp[i][j - 1]);
            }
        }
    }

    // Backtrack to identify matched reference indices
    std::vector<bool> ref_matched(m, false);
    size_t i = m;
    size_t j = n;

    while (i > 0 && j > 0) {
        if (!ref_norm[i - 1].empty() && ref_norm[i - 1] == trans_norm[j - 1]) {
            ref_matched[i - 1] = true;
            i--;
            j--;
        } else if (dp[i - 1][j] >= dp[i][j - 1]) {
            i--;
        } else {
            j--;
        }
    }

    // Token weighting classifier lambda
    auto classify_token = [](const std::string& orig_word, const std::string& norm_word, WordDiffToken& token) {
        // 1. Check if stopword
        static const std::vector<std::string> stopwords = {
            "el", "la", "los", "las", "un", "una", "unos", "unas", "de", "en", "por", "para", "con",
            "del", "al", "su", "sus", "y", "o", "que", "se", "es", "son", "a", "ante", "bajo", "cabe",
            "contra", "desde", "durante", "mediante", "hacia", "hasta", "sin", "sobre", "tras", "e", "u"
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
