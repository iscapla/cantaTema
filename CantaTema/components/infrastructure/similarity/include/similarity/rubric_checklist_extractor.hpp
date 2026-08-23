/**
 * @file rubric_checklist_extractor.hpp
 * @brief Automated exam rubric checklist extractor and multi-pass verification scorecard engine.
 */

#ifndef RUBRIC_CHECKLIST_EXTRACTOR_HPP
#define RUBRIC_CHECKLIST_EXTRACTOR_HPP

#include <string>
#include <vector>
#include <cstddef>
#include "primitives/definitions.hpp"

/**
 * @enum RubricEntityType
 * @brief Categorization of key academic and domain entities for exam rubric verification.
 */
enum class RubricEntityType {
    LEGAL_ARTICLE,      ///< Legal article references (e.g. "Artículo 14", "Art. 7")
    LAW_ID,             ///< Statute, decree, or law IDs (e.g. "Ley 39/2015", "RD 1/2020", "Estatuto")
    NUMERIC_METRIC,     ///< Quantitative metrics, percentages, amounts (e.g. "50%", "180.000 euros")
    DATE_OR_ERA,        ///< Calendar dates, centuries, historical milestones (e.g. "1978", "Siglo XX")
    SCIENTIFIC_TERM,    ///< Specialized scientific terminology and SI units (e.g. "ADN", "ATP", "Hz")
    ENUMERATED_POINT,   ///< Explicit ordered list items (e.g. "a)", "b)", "1º", "2º")
    DOMAIN_KEYWORD      ///< High-priority specialized domain keywords
};

/**
 * @struct RubricItem
 * @brief A single rubric entity extracted from reference material with verification status.
 */
struct RubricItem {
    size_t item_id{0};                                          ///< Unique checklist index.
    RubricEntityType entity_type{RubricEntityType::LEGAL_ARTICLE}; ///< Entity category type.
    std::string raw_text;                                       ///< Raw extracted text (e.g. "Artículo 14").
    std::string normalized_text;                                ///< Normalized token representation.
    size_t ref_chunk_index{0};                                  ///< Index of reference chunk where entity appeared.
    bool is_satisfied{false};                                   ///< True if spoken/verified in user transcript.
    int matched_transcript_index{-1};                           ///< Index of transcript segment that satisfied entity (-1 if unmentioned).
    float match_confidence{0.0f};                               ///< Match confidence rating (0.0 to 1.0).
    std::string entity_label;                                   ///< Human-readable rubric label.
};

/**
 * @struct RubricScorecard
 * @brief Aggregate rubric evaluation scorecard across all reference entities.
 */
struct RubricScorecard {
    size_t total_items{0};              ///< Total count of checklist entities.
    size_t satisfied_items{0};          ///< Count of satisfied / stated entities.
    size_t omitted_items{0};            ///< Count of missed / omitted entities.
    float citation_accuracy_pct{0.0f};  ///< Verification ratio: (satisfied_items / total_items) * 100.0%.
    std::vector<RubricItem> items;      ///< Ordered list of all rubric items.
};

/**
 * @class RubricChecklistExtractor
 * @brief Engine for extracting domain rubric entities and evaluating spoken candidate transcripts.
 */
class RubricChecklistExtractor {
public:
    /**
     * @brief Normalizes an input string for entity comparison.
     */
    static std::string normalize_text(const std::string& input);

    /**
     * @brief Extracts rubric checklist entities from a collection of reference document chunks.
     * 
     * @param ref_chunks Array of reference document chunk strings.
     * @param domain_key Domain profile key ("law", "economics", "science", "history", "general").
     * @return std::vector<RubricItem> Extracted rubric items.
     */
    static std::vector<RubricItem> extract_rubric_items(
        const std::vector<std::string>& ref_chunks,
        const std::string& domain_key = "law"
    );

    /**
     * @brief Evaluates an extracted rubric checklist against transcribed speech segments.
     * 
     * @param rubric_items The list of rubric items extracted from reference chunks.
     * @param transcript_segments The candidate transcript segments from speech recognition.
     * @param enable_phonetic Whether to use phonetic noise compensation for ASR mismatches.
     * @return RubricScorecard Detailed scorecard with satisfaction status per item.
     */
    static RubricScorecard evaluate_rubric(
        const std::vector<RubricItem>& rubric_items,
        const std::vector<std::string>& transcript_segments,
        bool enable_phonetic = true
    );
};

#endif // RUBRIC_CHECKLIST_EXTRACTOR_HPP
