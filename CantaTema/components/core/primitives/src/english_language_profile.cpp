/**
 * @file english_language_profile.cpp
 * @brief Implementation of EnglishLanguageProfile.
 */

#include "primitives/english_language_profile.hpp"
#include <algorithm>
#include <cctype>

EnglishLanguageProfile::EnglishLanguageProfile() {
    // Standard English stopwords
    std::vector<std::string> stopwords_list = {
        "the", "a", "an", "and", "or", "but", "if", "because", "as", "until", "while",
        "of", "at", "by", "for", "with", "about", "against", "between", "into", "through",
        "during", "before", "after", "above", "below", "to", "from", "up", "down", "in",
        "out", "on", "off", "over", "under", "again", "further", "then", "once", "here",
        "there", "when", "where", "why", "how", "all", "any", "both", "each", "few",
        "more", "most", "other", "some", "such", "no", "nor", "not", "only", "own",
        "same", "so", "than", "too", "very", "s", "t", "can", "will", "just", "don",
        "should", "now", "i", "me", "my", "myself", "we", "our", "ours", "ourselves",
        "you", "your", "yours", "yourself", "yourselves", "he", "him", "his", "himself",
        "she", "her", "hers", "herself", "it", "its", "itself", "they", "them", "their",
        "theirs", "themselves", "is", "am", "are", "was", "were", "be", "been", "being",
        "have", "has", "had", "having", "do", "does", "did", "doing"
    };

    for (const auto& w : stopwords_list) {
        m_stopwords.insert(w);
    }

    // Common English abbreviations
    std::vector<std::string> abbr_list = {
        "sec.", "sec", "art.", "art", "p.", "pp.", "e.g.", "i.e.", "no.", "vol.", "ch.",
        "chap.", "fig.", "ed.", "etc.", "etc"
    };

    for (const auto& a : abbr_list) {
        m_abbreviations.insert(a);
    }
}

std::string EnglishLanguageProfile::get_language_code() const {
    return "en";
}

std::string EnglishLanguageProfile::normalize_word(const std::string& input) const {
    std::string result;
    result.reserve(input.size());

    for (char c : input) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '.') {
            result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
    }

    if (result.size() > 1 && result.back() == '.' && m_abbreviations.find(result) == m_abbreviations.end()) {
        result.pop_back();
    }

    return result;
}

bool EnglishLanguageProfile::is_stopword(const std::string& word) const {
    std::string norm = normalize_word(word);
    return m_stopwords.find(norm) != m_stopwords.end();
}

bool EnglishLanguageProfile::is_abbreviation(const std::string& word) const {
    std::string norm = normalize_word(word);
    if (!norm.empty() && norm.back() != '.') {
        std::string with_dot = norm + ".";
        if (m_abbreviations.find(with_dot) != m_abbreviations.end()) {
            return true;
        }
    }
    return m_abbreviations.find(norm) != m_abbreviations.end();
}

std::vector<std::string> EnglishLanguageProfile::get_stopwords() const {
    return std::vector<std::string>(m_stopwords.begin(), m_stopwords.end());
}

std::vector<std::string> EnglishLanguageProfile::get_abbreviations() const {
    return std::vector<std::string>(m_abbreviations.begin(), m_abbreviations.end());
}
