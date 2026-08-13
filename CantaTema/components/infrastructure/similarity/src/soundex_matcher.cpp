/**
 * @file soundex_matcher.cpp
 * @brief Implementation of SoundexMatcher.
 */

#include "similarity/soundex_matcher.hpp"
#include "primitives/spanish_language_profile.hpp"
#include <algorithm>
#include <cctype>

std::string SoundexMatcher::get_matcher_id() const {
    return "soundex";
}

static std::string strip_accents(const std::string& input) {
    static SpanishLanguageProfile profile;
    return profile.normalize_word(input);
}

static char get_soundex_code(char c) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    switch (c) {
        case 'b': case 'f': case 'p': case 'v': return '1';
        case 'c': case 'g': case 'j': case 'k': case 'q': case 's': case 'x': case 'z': return '2';
        case 'd': case 't': return '3';
        case 'l': return '4';
        case 'm': case 'n': return '5';
        case 'r': return '6';
        default: return '0';
    }
}

std::string SoundexMatcher::get_phonetic_code(const std::string& input_word) const {
    std::string word = strip_accents(input_word);
    if (word.empty()) return "0000";

    std::string code;
    code.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(word[0]))));

    char prev_code = get_soundex_code(word[0]);

    for (size_t i = 1; i < word.size() && code.size() < 4; ++i) {
        char current_code = get_soundex_code(word[i]);
        if (current_code != '0' && current_code != prev_code) {
            code.push_back(current_code);
        }
        if (current_code != '0') {
            prev_code = current_code;
        }
    }

    while (code.size() < 4) {
        code.push_back('0');
    }

    return code;
}

PhoneticMatchResult SoundexMatcher::compare_words(const std::string& word1, const std::string& word2) const {
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

    std::string code1 = get_phonetic_code(word1);
    std::string code2 = get_phonetic_code(word2);

    result.primary_code = code1;
    result.secondary_code = code2;

    if (code1 == code2) {
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
