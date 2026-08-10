/**
 * @file i_text_comparison_visualizer.hpp
 * @brief Abstract interface and data models for dual-column text comparison HTML report visualizers.
 */

#ifndef I_TEXT_COMPARISON_VISUALIZER_HPP
#define I_TEXT_COMPARISON_VISUALIZER_HPP

#include <string>
#include <vector>
#include <cstdint>
#include "primitives/definitions.hpp"

#include "similarity/word_sequence_aligner.hpp"

/**
 * @enum coverage_level_e
 * @brief Coverage match status levels for reference document chunks.
 */
enum class coverage_level_e {
    MENTIONED,     ///< Level 1: Mentioned in audio (Green)
    NOT_CLEAR,     ///< Level 2: Partial or low similarity match (Yellow)
    NOT_MENTIONED  ///< Level 3: Omitted from practice audio (Red)
};

/**
 * @struct TextComparisonInput
 * @brief Input payload containing reference document chunks, spoken transcript segments, and similarity mappings.
 */
struct TextComparisonInput {
    std::string document_title;           ///< Display title of reference text document.
    std::string reference_filepath;       ///< File path to source PDF/text reference document.
    std::string audio_filepath;           ///< File path to recorded user audio practice session.
    std::string whisper_model;            ///< Identifier of Whisper speech recognition model.
    std::string llama_model;              ///< Identifier of llama.cpp embedding model.

    double overall_coverage_pct{0.0};     ///< Computed overall percentage of reference covered (0.0 - 100.0%).
    size_t total_ref_chunks{0};           ///< Total count of sentence/paragraph reference chunks.
    size_t mentioned_chunks{0};           ///< Count of reference chunks matching MENTIONED status.
    size_t not_clear_chunks{0};           ///< Count of reference chunks matching NOT_CLEAR status.
    size_t not_mentioned_chunks{0};       ///< Count of reference chunks matching NOT_MENTIONED status.
    float threshold_mentioned{0.75f};     ///< Similarity threshold for MENTIONED classification.
    float threshold_not_clear{0.50f};     ///< Similarity threshold for NOT_CLEAR classification.

    /**
     * @struct ReferenceItem
     * @brief A single chunk/sentence from reference document with its similarity match state.
     */
    struct ReferenceItem {
        size_t id{0};                                                     ///< Unique chunk identifier index.
        std::string text;                                                 ///< Text content of chunk.
        float importance_weight{1.0f};                                    ///< Weight score based on text formatting (bold, italic, etc).
        coverage_level_e coverage_status{coverage_level_e::NOT_MENTIONED};///< Classified coverage status level.
        float similarity_score{0.0f};                                     ///< Maximum vector similarity score against transcript.
        int matched_transcript_index{-1};                                 ///< Index of best matching transcript item (-1 if unmentioned).
        WordAlignmentResult word_alignment;                               ///< Word-level diff breakdown (matched vs omitted words).
        float word_recall_score{0.0f};                                    ///< Percentage of reference words spoken (0.0 to 1.0).
    };
    std::vector<ReferenceItem> reference_items;                           ///< Collection of reference document items.

    /**
     * @struct TranscriptItem
     * @brief A single transcribed audio segment with timecodes and similarity link.
     */
    struct TranscriptItem {
        size_t id{0};                        ///< Unique transcript item index.
        uint64_t start_time_ms{0};           ///< Audio start timestamp in milliseconds.
        uint64_t end_time_ms{0};             ///< Audio end timestamp in milliseconds.
        std::string text;                    ///< Spoken text content.
        float confidence_score{1.0f};        ///< Confidence rating of Whisper transcription (0.0 - 1.0).
        int primary_matched_ref_index{-1};   ///< Index of reference item this segment primary matches (-1 if none).
    };
    std::vector<TranscriptItem> transcript_items;                         ///< Collection of spoken transcript items.
};

/**
 * @class ITextComparisonVisualizer
 * @brief Abstract interface for generating dual-column text comparison HTML reports.
 */
class ITextComparisonVisualizer {
public:
    /**
     * @brief Virtual destructor for ITextComparisonVisualizer.
     */
    virtual ~ITextComparisonVisualizer() = default;

    /**
     * @brief Generates a 2-column side-by-side comparison HTML document string with bidirectional hover highlighting and synchronized scrolling.
     * @param input Structure containing reference text chunks, voice transcript segments, and similarity mappings.
     * @param out_html_content Output string receiving the full generated HTML.
     * @return rst_code_e RST_OK on success, or error code.
     */
    virtual rst_code_e generate_html(const TextComparisonInput& input, std::string& out_html_content) const = 0;
};

#endif // I_TEXT_COMPARISON_VISUALIZER_HPP
