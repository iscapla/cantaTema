/**
 * @file domain_profiles.cpp
 * @brief Implementations of subject domain profiles (Law, Economics, History, Science, General).
 */

#include "primitives/domain_profiles.hpp"
#include "primitives/spanish_language_profile.hpp"
#include <algorithm>
#include <cctype>

static bool is_numeric_string(const std::string& str) {
    if (str.empty()) return false;
    bool has_digit = false;
    for (char c : str) {
        if (std::isdigit(static_cast<unsigned char>(c))) {
            has_digit = true;
        } else if (c != '.' && c != ',' && c != '%' && c != '/' && c != '-') {
            return false;
        }
    }
    return has_digit;
}

static bool is_capitalized(const std::string& str) {
    if (str.empty()) return false;
    return std::isupper(static_cast<unsigned char>(str[0])) != 0;
}

static std::string normalize_token_for_domain(const std::string& input) {
    static SpanishLanguageProfile spanish_profile;
    return spanish_profile.normalize_word(input);
}


// ------------------- LawDomainProfile -------------------

LawDomainProfile::LawDomainProfile() {
    m_citation_keywords = {
        "articulo", "art.", "art", "ley", "rd", "r.d.", "estatuto", "reglamento",
        "constitucion", "titulo", "capitulo", "seccion", "disposicion", "decreto"
    };
}

std::string LawDomainProfile::get_domain_key() const { return "law"; }
std::string LawDomainProfile::get_domain_name() const { return "Law & Legal Studies"; }

bool LawDomainProfile::is_high_priority_citation(const std::string& token) const {
    std::string norm = normalize_token_for_domain(token);
    return m_citation_keywords.find(norm) != m_citation_keywords.end();
}


float LawDomainProfile::get_token_weight(const std::string& token, bool is_stopword) const {
    if (is_stopword) return 0.2f;
    if (is_high_priority_citation(token)) return 4.0f;
    if (is_numeric_string(token)) return 3.0f;
    if (is_capitalized(token)) return 2.0f;
    return 1.0f;
}

std::string LawDomainProfile::get_missing_warning_badge() const {
    return "⚠️ MISSING LEGAL CITATION";
}

// ------------------- EconomicsDomainProfile -------------------

EconomicsDomainProfile::EconomicsDomainProfile() {
    m_economic_indicators = {
        "pib", "gdp", "ipc", "cpi", "inflacion", "inflation", "deficit", "superavit",
        "balance", "monetarismo", "fisiocracia", "tasas", "interes", "poblacion", "desempleo"
    };
}

std::string EconomicsDomainProfile::get_domain_key() const { return "economics"; }
std::string EconomicsDomainProfile::get_domain_name() const { return "Economics & Finance"; }

bool EconomicsDomainProfile::is_high_priority_citation(const std::string& token) const {
    std::string lower = token;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (m_economic_indicators.find(lower) != m_economic_indicators.end()) return true;
    if (token.find('%') != std::string::npos || lower.find("euro") != std::string::npos || lower.find("dolar") != std::string::npos || token.find('$') != std::string::npos) {
        return true;
    }
    return false;
}

float EconomicsDomainProfile::get_token_weight(const std::string& token, bool is_stopword) const {
    if (is_stopword) return 0.2f;
    if (is_high_priority_citation(token)) return 4.0f;
    if (is_numeric_string(token)) return 3.0f;
    if (is_capitalized(token)) return 2.0f;
    return 1.0f;
}

std::string EconomicsDomainProfile::get_missing_warning_badge() const {
    return "⚠️ MISSING ECONOMIC INDICATOR";
}

// ------------------- HistoryDomainProfile -------------------

HistoryDomainProfile::HistoryDomainProfile() {
    m_history_keywords = {
        "siglo", "century", "tratado", "treaty", "pacto", "dinastia", "guerra", "war",
        "revolucion", "imperio", "empire", "batalla", "reino", "constitucion"
    };
}

std::string HistoryDomainProfile::get_domain_key() const { return "history"; }
std::string HistoryDomainProfile::get_domain_name() const { return "History & World Events"; }

bool HistoryDomainProfile::is_high_priority_citation(const std::string& token) const {
    std::string lower = token;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (m_history_keywords.find(lower) != m_history_keywords.end()) return true;
    if (is_numeric_string(token) && token.size() == 4) return true; // Year (e.g. 1492, 1789)
    return false;
}

float HistoryDomainProfile::get_token_weight(const std::string& token, bool is_stopword) const {
    if (is_stopword) return 0.2f;
    if (is_high_priority_citation(token)) return 4.0f;
    if (is_capitalized(token) || is_numeric_string(token)) return 3.0f;
    return 1.0f;
}

std::string HistoryDomainProfile::get_missing_warning_badge() const {
    return "⚠️ MISSING HISTORICAL ERA/DATE";
}

// ------------------- ScienceDomainProfile -------------------

ScienceDomainProfile::ScienceDomainProfile() {
    m_science_terms = {
        "adn", "dna", "arn", "rna", "atp", "mitocondria", "isotopo", "teorema",
        "celula", "atomo", "molecula", "gen", "proteina", "enzima", "gravedad"
    };

    m_si_units = {
        "kg", "m/s", "m/s2", "hz", "mol", "joule", "watt", "volt", "ampere", "kelvin", "pascal"
    };
}

std::string ScienceDomainProfile::get_domain_key() const { return "science"; }
std::string ScienceDomainProfile::get_domain_name() const { return "Sciences (Physics/Chem/Bio)"; }

bool ScienceDomainProfile::is_high_priority_citation(const std::string& token) const {
    std::string lower = token;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (m_science_terms.find(lower) != m_science_terms.end()) return true;
    if (m_si_units.find(lower) != m_si_units.end()) return true;
    return false;
}

float ScienceDomainProfile::get_token_weight(const std::string& token, bool is_stopword) const {
    if (is_stopword) return 0.2f;
    if (is_high_priority_citation(token)) return 4.0f;
    if (is_numeric_string(token)) return 3.0f;
    if (is_capitalized(token)) return 2.0f;
    return 1.0f;
}

std::string ScienceDomainProfile::get_missing_warning_badge() const {
    return "⚠️ MISSING SCIENTIFIC TERM/UNIT";
}

// ------------------- GeneralDomainProfile -------------------

GeneralDomainProfile::GeneralDomainProfile() {}

std::string GeneralDomainProfile::get_domain_key() const { return "general"; }
std::string GeneralDomainProfile::get_domain_name() const { return "General Literature & Essays"; }

bool GeneralDomainProfile::is_high_priority_citation(const std::string& token) const {
    return is_capitalized(token) || is_numeric_string(token);
}

float GeneralDomainProfile::get_token_weight(const std::string& token, bool is_stopword) const {
    if (is_stopword) return 0.2f;
    if (is_capitalized(token) || is_numeric_string(token)) return 3.0f;
    return 1.0f;
}

std::string GeneralDomainProfile::get_missing_warning_badge() const {
    return "⚠️ MISSING KEYWORD";
}
