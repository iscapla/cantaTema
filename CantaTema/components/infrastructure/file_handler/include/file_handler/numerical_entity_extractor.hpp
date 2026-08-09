/**
 * @file numerical_entity_extractor.hpp
 * @brief Header for numerical and temporal entity extraction and list structure identification.
 */

#ifndef NUMERICAL_ENTITY_EXTRACTOR_HPP
#define NUMERICAL_ENTITY_EXTRACTOR_HPP

#include <string>
#include <vector>
#include <unordered_set>

/**
 * @enum NumericResult
 * @brief Categorization of numerical entity comparisons between reference text and transcript text.
 */
enum class NumericResult {
    NONE,                   ///< No numerical entities present in reference chunk.
    EXACT_MATCH,            ///< Reference numerical entities match transcript entities.
    MISMATCH,               ///< Transcript contains conflicting numerical entities (misquoted number/date).
    MISSING_IN_TRANSCRIPT   ///< Reference chunk has numerical entities but transcript omitted them.
};

/**
 * @struct NumericMatchAnalysis
 * @brief Holds analysis output for numerical entity comparison.
 */
struct NumericMatchAnalysis {
    NumericResult status = NumericResult::NONE;
    bool has_warning = false;
    float score_modifier = 0.0f;
};

/**
 * @class NumericalEntityExtractor
 * @brief Provides utility methods for parsing dates, numbers, articles, percentages,
 * and identifying enumerated list item structures.
 */
class NumericalEntityExtractor {
public:
    NumericalEntityExtractor() = default;
    ~NumericalEntityExtractor() = default;

    /**
     * @brief Checks if a string represents an enumerated point, bullet item, or list entry.
     * 
     * @param text The input text string.
     * @return true if the string starts with a list marker (e.g. "1.", "a)", "•", "1º", "Artículo 5").
     * @return false otherwise.
     */
    static bool is_enumerated_item(const std::string& text);

    /**
     * @brief Extracts normalized numerical and temporal tokens from text.
     * 
     * @param text Input text string.
     * @return std::unordered_set<std::string> Set of extracted numerical tokens (e.g. "1978", "14", "15%").
     */
    static std::unordered_set<std::string> extract_entities(const std::string& text);

    /**
     * @brief Compares numerical entities between a reference chunk and a transcript segment.
     * 
     * @param ref_text The reference chunk text.
     * @param transcript_text The candidate transcript segment text.
     * @param boost Score boost applied when numerical entities match exactly (e.g. +0.10).
     * @param penalty Score penalty applied when numerical entities conflict (e.g. -0.15).
     * @return NumericMatchAnalysis Detailed analysis result including warning flag and score modifier.
     */
    static NumericMatchAnalysis compare_entities(
        const std::string& ref_text,
        const std::string& transcript_text,
        float boost = 0.10f,
        float penalty = 0.15f
    );
};

#endif // NUMERICAL_ENTITY_EXTRACTOR_HPP
