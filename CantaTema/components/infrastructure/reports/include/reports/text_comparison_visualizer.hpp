/**
 * @file text_comparison_visualizer.hpp
 * @brief Concrete HTML visualizer for dual-column reference text vs. spoken transcript comparison reports.
 */

#ifndef TEXT_COMPARISON_VISUALIZER_HPP
#define TEXT_COMPARISON_VISUALIZER_HPP

#include "reports/i_text_comparison_visualizer.hpp"

/**
 * @class TextComparisonVisualizer
 * @brief Concrete implementation of ITextComparisonVisualizer.
 *
 * Generates an interactive 2-column side-by-side comparison HTML document with:
 * - Reference Document (Left Column) color-coded by coverage level (Mentioned, Not Clear, Not Mentioned).
 * - Voice Transcript (Right Column).
 * - Bidirectional auto-highlighting when moving cursor over matching fragments.
 * - Synchronized dual-column auto-scrolling to keep aligned fragments in view.
 */
class TextComparisonVisualizer : public ITextComparisonVisualizer {
public:
    /**
     * @brief Constructs a TextComparisonVisualizer object.
     */
    TextComparisonVisualizer() = default;

    /**
     * @brief Destroys the TextComparisonVisualizer object.
     */
    ~TextComparisonVisualizer() override = default;

    /**
     * @brief Generates a 2-column side-by-side comparison HTML report string with bidirectional hover highlighting and synchronized scrolling.
     * @param input Data payload containing reference text chunks, spoken transcript segments, and similarity mappings.
     * @param out_html_content Output string buffer where generated HTML content will be stored.
     * @return rst_code_e RST_OK on success, or appropriate error code on failure.
     */
    rst_code_e generate_html(const TextComparisonInput& input, std::string& out_html_content) const override;

private:
    /**
     * @brief Escapes HTML special characters in a string to prevent XSS / markup injection.
     * @param str The raw input string to be escaped.
     * @return std::string The HTML-safe escaped string.
     */
    static std::string escape_html(const std::string& str);

    /**
     * @brief Formats a timestamp duration in milliseconds into a MM:SS.mmm time format string.
     * @param ms Timestamp duration in milliseconds.
     * @return std::string Formatted time string (e.g. "01:23.456").
     */
    static std::string format_timestamp(uint64_t ms);
};

#endif // TEXT_COMPARISON_VISUALIZER_HPP
