/**
 * @file lexical_keyword_extractor.cpp
 * @brief Implementation of LexicalKeywordExtractor methods.
 */

#include "file_handler/lexical_keyword_extractor.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

static const std::unordered_set<std::string> SPANISH_STOP_WORDS = {
    "el", "la", "los", "las", "un", "una", "unos", "unas",
    "de", "del", "a", "al", "ante", "bajo", "con", "contra", "desde", "en", "entre",
    "hacia", "hasta", "para", "por", "según", "sin", "sobre", "tras",
    "y", "o", "u", "e", "pero", "sino", "que", "si", "no", "ni",
    "su", "sus", "mi", "mis", "tu", "tus", "nuestro", "nuestra", "nuestros", "nuestras",
    "este", "esta", "estos", "estas", "ese", "esa", "esos", "esas", "aquel", "aquella",
    "cuando", "donde", "como", "cual", "quien", "cuanto", "mas", "más", "pero",
    "por", "para", "porque", "como", "así", "asi", "también", "tambien", "hubiera", "hubiese",
    "ser", "estar", "haber", "tener", "hacer", "fue", "era", "son", "es", "han", "ha"
};

static std::string sanitize_word(const std::string& word) {
    std::string clean = "";
    for (char c : word) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            clean += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
    }
    return clean;
}

size_t LexicalKeywordExtractor::count_words(const std::string& text) {
    std::stringstream ss(text);
    std::string word;
    size_t count = 0;
    while (ss >> word) {
        if (!sanitize_word(word).empty()) {
            count++;
        }
    }
    return count;
}

std::unordered_set<std::string> LexicalKeywordExtractor::extract_keywords(const std::string& text) {
    std::unordered_set<std::string> keywords;
    std::stringstream ss(text);
    std::string token;

    while (ss >> token) {
        std::string clean = sanitize_word(token);
        if (clean.length() >= 3 && SPANISH_STOP_WORDS.count(clean) == 0) {
            keywords.insert(clean);
        }
    }

    return keywords;
}

size_t LexicalKeywordExtractor::count_keyword_overlap(
    const std::unordered_set<std::string>& ref_keywords,
    const std::unordered_set<std::string>& trans_keywords
) {
    if (ref_keywords.empty() || trans_keywords.empty()) {
        return 0;
    }

    size_t matches = 0;
    for (const auto& kw : ref_keywords) {
        if (trans_keywords.count(kw) > 0) {
            matches++;
        }
    }
    return matches;
}
