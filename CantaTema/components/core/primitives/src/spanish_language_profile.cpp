/**
 * @file spanish_language_profile.cpp
 * @brief Implementation of SpanishLanguageProfile.
 */

#include "primitives/spanish_language_profile.hpp"
#include <algorithm>
#include <cctype>

SpanishLanguageProfile::SpanishLanguageProfile() {
    // Standard Spanish stopwords
    std::vector<std::string> stopwords_list = {
        "el", "la", "los", "las", "un", "una", "unos", "unas", "de", "del", "a", "al",
        "en", "con", "por", "para", "sin", "sobre", "entre", "tras", "durante", "hasta",
        "y", "e", "ni", "o", "u", "pero", "mas", "sino", "que", "como", "cuando", "donde",
        "su", "sus", "mi", "mis", "tu", "tus", "nuestro", "nuestra", "nuestros", "nuestras",
        "este", "esta", "estos", "estas", "ese", "esa", "esos", "esas", "aquel", "aquella",
        "aquellos", "aquellas", "yo", "tu", "el", "ella", "ellos", "ellas", "nosotros", "nosotras",
        "vosotros", "vosotras", "se", "me", "te", "nos", "os", "lo", "le", "les", "que", "quien",
        "quienes", "cual", "cuales", "cuyo", "cuyos", "cuya", "cuyas", "es", "son", "fue", "fueron",
        "ha", "han", "habia", "habian", "ser", "estar", "tener", "hacer"
    };

    for (const auto& w : stopwords_list) {
        m_stopwords.insert(w);
    }

    // Common Spanish legal & general abbreviations (lowercase normalized)
    std::vector<std::string> abbr_list = {
        "art.", "art", "sec.", "sec", "p.ag.", "pag.", "pag", "r.d.", "rd", "no.", "num.",
        "num", "vol.", "cap.", "cap", "tit.", "tit", "apda.", "apdo.", "apdo", "disp.",
        "ed.", "etc.", "etc", "e.g.", "i.e."
    };

    for (const auto& a : abbr_list) {
        m_abbreviations.insert(a);
    }
}

std::string SpanishLanguageProfile::get_language_code() const {
    return "es";
}

std::string SpanishLanguageProfile::normalize_word(const std::string& input) const {
    std::string result;
    result.reserve(input.size());

    // Simple UTF-8 aware normalization for Spanish
    size_t i = 0;
    while (i < input.size()) {
        unsigned char c = static_cast<unsigned char>(input[i]);
        if (c == 0xC3 && i + 1 < input.size()) {
            unsigned char c2 = static_cast<unsigned char>(input[i + 1]);
            // Accented vowels replace with plain ASCII
            switch (c2) {
                case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: // Á, Á, etc.
                case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4: case 0xA5: // á, á, etc.
                    result.push_back('a'); break;
                case 0x88: case 0x89: case 0x8A: case 0x8B: // É
                case 0xA8: case 0xA9: case 0xAA: case 0xAB: // é
                    result.push_back('e'); break;
                case 0x8C: case 0x8D: case 0x8E: case 0x8F: // Í
                case 0xAC: case 0xAD: case 0xAE: case 0xAF: // í
                    result.push_back('i'); break;
                case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: // Ó, Ö
                case 0xB2: case 0xB3: case 0xB4: case 0xB5: case 0xB6: // ó, ö
                    result.push_back('o'); break;
                case 0x99: case 0x9A: case 0x9B: case 0x9C: // Ú, Ü
                case 0xB9: case 0xBA: case 0xBB: case 0xBC: // ú, ü
                    result.push_back('u'); break;
                default:
                    // Preserve ñ (C3 91 / C3 B1) and other multibyte characters as-is
                    result.push_back(input[i]);
                    result.push_back(input[i + 1]);
                    break;
            }
            i += 2;
        } else if (c == 0xC2 && i + 1 < input.size()) {
            unsigned char c2 = static_cast<unsigned char>(input[i + 1]);
            if (c2 == 0xBF || c2 == 0xA1) {
                // Strip ¿ (C2 BF) and ¡ (C2 A1)
                i += 2;
            } else {
                result.push_back(input[i]);
                result.push_back(input[i + 1]);
                i += 2;
            }
        } else {
            if (std::isalnum(c) || c == '.') {
                result.push_back(static_cast<char>(std::tolower(c)));
            }
            i++;
        }
    }

    // Strip trailing dot if not an abbreviation
    if (result.size() > 1 && result.back() == '.' && m_abbreviations.find(result) == m_abbreviations.end()) {
        result.pop_back();
    }

    return result;
}

bool SpanishLanguageProfile::is_stopword(const std::string& word) const {
    std::string norm = normalize_word(word);
    return m_stopwords.find(norm) != m_stopwords.end();
}

bool SpanishLanguageProfile::is_abbreviation(const std::string& word) const {
    std::string norm = normalize_word(word);
    if (!norm.empty() && norm.back() != '.') {
        std::string with_dot = norm + ".";
        if (m_abbreviations.find(with_dot) != m_abbreviations.end()) {
            return true;
        }
    }
    return m_abbreviations.find(norm) != m_abbreviations.end();
}

std::vector<std::string> SpanishLanguageProfile::get_stopwords() const {
    return std::vector<std::string>(m_stopwords.begin(), m_stopwords.end());
}

std::vector<std::string> SpanishLanguageProfile::get_abbreviations() const {
    return std::vector<std::string>(m_abbreviations.begin(), m_abbreviations.end());
}
