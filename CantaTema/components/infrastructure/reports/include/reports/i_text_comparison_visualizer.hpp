#ifndef I_TEXT_COMPARISON_VISUALIZER_HPP
#define I_TEXT_COMPARISON_VISUALIZER_HPP

#include <string>
#include <vector>
#include <cstdint>
#include "primitives/definitions.hpp"

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
    std::string document_title;
    std::string reference_filepath;
    std::string audio_filepath;
    std::string whisper_model;
    std::string llama_model;

    double overall_coverage_pct{0.0};
    size_t total_ref_chunks{0};
    size_t mentioned_chunks{0};
    size_t not_clear_chunks{0};
    size_t not_mentioned_chunks{0};
    float threshold_mentioned{0.75f};
    float threshold_not_clear{0.50f};

    struct ReferenceItem {
        size_t id{0};
        std::string text;
        float importance_weight{1.0f};
        coverage_level_e coverage_status{coverage_level_e::NOT_MENTIONED};
        float similarity_score{0.0f};
        int matched_transcript_index{-1};
    };
    std::vector<ReferenceItem> reference_items;

    struct TranscriptItem {
        size_t id{0};
        uint64_t start_time_ms{0};
        uint64_t end_time_ms{0};
        std::string text;
        float confidence_score{1.0f};
        int primary_matched_ref_index{-1};
    };
    std::vector<TranscriptItem> transcript_items;
};

/**
 * @class ITextComparisonVisualizer
 * @brief Abstract interface for generating dual-column text comparison HTML reports.
 */
class ITextComparisonVisualizer {
public:
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
