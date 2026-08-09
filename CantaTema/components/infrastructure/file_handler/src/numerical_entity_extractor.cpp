/**
 * @file numerical_entity_extractor.cpp
 * @brief Implementation of NumericalEntityExtractor methods.
 */

#include "file_handler/numerical_entity_extractor.hpp"
#include <regex>
#include <algorithm>
#include <cctype>

static std::string to_lower_ascii(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return result;
}

bool NumericalEntityExtractor::is_enumerated_item(const std::string& text) {
    if (text.empty()) {
        return false;
    }

    // Trim leading whitespace
    size_t start = 0;
    while (start < text.length() && std::isspace(static_cast<unsigned char>(text[start]))) {
        start++;
    }
    if (start >= text.length()) {
        return false;
    }

    std::string trimmed = text.substr(start);

    // Regex for list markers: "1.", "1)", "1.-", "(1)", "a.", "a)", "•", "-", "*", "1º", "2º", "Primero", "Segundo", "Artículo 5", "Art. 5"
    static const std::regex list_marker_regex(
        R"(^(\d+[\.\)\-]|[a-zA-Z][\.\)]|\(\d+\)|\([a-zA-Z]\)|[•\-\*]|[0-9]+[ºª]|artículo\s+\d+|art\.\s*\d+|primero|segundo|tercero|cuarto|quinto|sexto|séptimo|octavo|noveno|décimo))",
        std::regex_constants::icase
    );

    return std::regex_search(trimmed, list_marker_regex);
}

std::unordered_set<std::string> NumericalEntityExtractor::extract_entities(const std::string& text) {
    std::unordered_set<std::string> entities;
    if (text.empty()) {
        return entities;
    }

    std::string lower_text = to_lower_ascii(text);

    // 1. Match percentages (e.g., "15%", "14.5%")
    static const std::regex percent_regex(R"(\b\d+(\.\d+)?%\b)");
    auto percent_begin = std::sregex_iterator(lower_text.begin(), lower_text.end(), percent_regex);
    auto percent_end = std::sregex_iterator();
    for (auto it = percent_begin; it != percent_end; ++it) {
        entities.insert(it->str());
    }

    // 2. Match dates formatted like DD/MM/YYYY or YYYY-MM-DD
    static const std::regex date_regex(R"(\b\d{1,2}[\/\-]\d{1,2}[\/\-]\d{2,4}\b|\b\d{4}[\/\-]\d{1,2}[\/\-]\d{1,2}\b)");
    auto date_begin = std::sregex_iterator(lower_text.begin(), lower_text.end(), date_regex);
    auto date_end = std::sregex_iterator();
    for (auto it = date_begin; it != date_end; ++it) {
        entities.insert(it->str());
    }

    // 3. Match article / section identifiers (e.g., "artículo 14", "art. 14", "art 14")
    static const std::regex article_regex(R"(\bart(?:ículo)?\s*(\d+)\b)");
    auto art_begin = std::sregex_iterator(lower_text.begin(), lower_text.end(), article_regex);
    auto art_end = std::sregex_iterator();
    for (auto it = art_begin; it != art_end; ++it) {
        if (it->size() > 1) {
            entities.insert("art" + (*it)[1].str());
        }
    }

    // 4. Match standalone numbers/years (2+ digits or single digits in numeric context)
    static const std::regex number_regex(R"(\b\d+\b)");
    auto num_begin = std::sregex_iterator(lower_text.begin(), lower_text.end(), number_regex);
    auto num_end = std::sregex_iterator();
    for (auto it = num_begin; it != num_end; ++it) {
        std::string val = it->str();
        // Ignore single digit numbers unless part of a short text
        if (val.length() >= 2 || text.length() < 20) {
            entities.insert(val);
        }
    }

    return entities;
}

NumericMatchAnalysis NumericalEntityExtractor::compare_entities(
    const std::string& ref_text,
    const std::string& transcript_text,
    float boost,
    float penalty
) {
    NumericMatchAnalysis analysis;
    
    auto ref_entities = extract_entities(ref_text);
    if (ref_entities.empty()) {
        analysis.status = NumericResult::NONE;
        analysis.has_warning = false;
        analysis.score_modifier = 0.0f;
        return analysis;
    }

    auto trans_entities = extract_entities(transcript_text);
    if (trans_entities.empty()) {
        analysis.status = NumericResult::MISSING_IN_TRANSCRIPT;
        analysis.has_warning = false;
        analysis.score_modifier = -0.5f * penalty; // Mild penalty when audio omits numbers
        return analysis;
    }

    size_t matches = 0;
    size_t conflicts = 0;

    for (const auto& entity : ref_entities) {
        if (trans_entities.count(entity) > 0) {
            matches++;
        } else {
            // Check if there are other numerical entities present in transcript that conflict
            conflicts++;
        }
    }

    if (matches > 0 && conflicts == 0) {
        analysis.status = NumericResult::EXACT_MATCH;
        analysis.has_warning = false;
        analysis.score_modifier = boost;
    } else if (matches > 0 && conflicts > 0) {
        // Partial match with some conflicting numbers -> Warning status!
        analysis.status = NumericResult::MISMATCH;
        analysis.has_warning = true;
        analysis.score_modifier = boost - penalty;
    } else if (matches == 0 && conflicts > 0) {
        // Complete mismatch (misquoted date/number) -> Warning status!
        analysis.status = NumericResult::MISMATCH;
        analysis.has_warning = true;
        analysis.score_modifier = -penalty;
    } else {
        analysis.status = NumericResult::MISSING_IN_TRANSCRIPT;
        analysis.has_warning = false;
        analysis.score_modifier = -0.5f * penalty;
    }

    return analysis;
}
