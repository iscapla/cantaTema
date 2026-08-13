/**
 * @file double_metaphone_matcher.cpp
 * @brief Implementation of DoubleMetaphoneMatcher.
 */

#include "similarity/double_metaphone_matcher.hpp"
#include "primitives/spanish_language_profile.hpp"
#include <algorithm>
#include <cctype>

std::string DoubleMetaphoneMatcher::get_matcher_id() const {
    return "double_metaphone";
}

static std::string strip_accents(const std::string& input) {
    static SpanishLanguageProfile profile;
    return profile.normalize_word(input);
}

std::pair<std::string, std::string> DoubleMetaphoneMatcher::compute_double_metaphone(const std::string& input_word) const {
    std::string word = strip_accents(input_word);
    if (word.empty()) return {"", ""};

    std::string primary;
    std::string secondary;
    primary.reserve(word.size());
    secondary.reserve(word.size());

    // Simplified Double Metaphone phonetic rules tuned for Spanish & English speech
    for (size_t i = 0; i < word.size(); ++i) {
        char c = word[i];
        switch (c) {
            case 'b': case 'v':
                primary.push_back('P');
                secondary.push_back('P');
                break;
            case 'c': case 'z':
                if (i + 1 < word.size() && (word[i + 1] == 'e' || word[i + 1] == 'i')) {
                    primary.push_back('S');
                    secondary.push_back('S');
                } else if (i + 1 < word.size() && word[i + 1] == 'h') {
                    primary.push_back('X');
                    secondary.push_back('X');
                    i++;
                } else {
                    primary.push_back('K');
                    secondary.push_back('K');
                }
                break;
            case 'd':
                primary.push_back('T');
                secondary.push_back('T');
                break;
            case 'g': case 'j':
                if (i + 1 < word.size() && (word[i + 1] == 'e' || word[i + 1] == 'i')) {
                    primary.push_back('J');
                    secondary.push_back('H');
                } else {
                    primary.push_back('K');
                    secondary.push_back('K');
                }
                break;
            case 'h':
                // Silent in Spanish unless preceded by c
                if (i == 0) {
                    primary.push_back('H');
                }
                break;
            case 'k': case 'q':
                primary.push_back('K');
                secondary.push_back('K');
                break;
            case 'l':
                if (i + 1 < word.size() && word[i + 1] == 'l') {
                    primary.push_back('Y');
                    secondary.push_back('L');
                    i++;
                } else {
                    primary.push_back('L');
                    secondary.push_back('L');
                }
                break;
            case 'm': case 'n':
                primary.push_back('M');
                secondary.push_back('M');
                break;
            case 'p':
                primary.push_back('P');
                secondary.push_back('P');
                break;
            case 'r':
                primary.push_back('R');
                secondary.push_back('R');
                break;
            case 's': case 'x':
                primary.push_back('S');
                secondary.push_back('X');
                break;
            case 't':
                primary.push_back('T');
                secondary.push_back('T');
                break;
            case 'a': case 'e': case 'i': case 'o': case 'u':
                if (i == 0) {
                    primary.push_back('A');
                    secondary.push_back('A');
                }
                break;
            default:
                break;
        }
    }

    if (primary.size() > 6) primary.resize(6);
    if (secondary.size() > 6) secondary.resize(6);

    return {primary, secondary};
}

std::string DoubleMetaphoneMatcher::get_phonetic_code(const std::string& word) const {
    auto dm = compute_double_metaphone(word);
    return dm.first;
}

PhoneticMatchResult DoubleMetaphoneMatcher::compare_words(const std::string& word1, const std::string& word2) const {
    PhoneticMatchResult result;
    std::string n1 = strip_accents(word1);
    std::string n2 = strip_accents(word2);

    if (n1 == n2) {
        result.is_match = true;
        result.similarity_score = 1.0f;
        result.is_minor_mispronunciation = false;
        result.primary_code = get_phonetic_code(word1);
        return result;
    }

    auto dm1 = compute_double_metaphone(word1);
    auto dm2 = compute_double_metaphone(word2);

    result.primary_code = dm1.first;
    result.secondary_code = dm1.second;

    bool match_p = (!dm1.first.empty() && (dm1.first == dm2.first || dm1.first == dm2.second));
    bool match_s = (!dm1.second.empty() && (dm1.second == dm2.first || dm1.second == dm2.second));

    if (match_p || match_s) {
        result.is_match = true;
        result.similarity_score = 0.85f;
        result.is_minor_mispronunciation = true;
    } else {
        result.is_match = false;
        result.similarity_score = 0.0f;
        result.is_minor_mispronunciation = false;
    }

    return result;
}
