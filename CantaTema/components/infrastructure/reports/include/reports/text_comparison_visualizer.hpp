#ifndef TEXT_COMPARISON_VISUALIZER_HPP
#define TEXT_COMPARISON_VISUALIZER_HPP

#include "reports/i_text_comparison_visualizer.hpp"

/**
 * @class TextComparisonVisualizer
 * @brief Concrete implementation of ITextComparisonVisualizer.
 * Generates an interactive 2-column side-by-side comparison HTML document with:
 * - Reference Document (Left Column) color-coded by coverage level (Mentioned, Not Clear, Not Mentioned).
 * - Voice Transcript (Right Column).
 * - Bidirectional auto-highlighting when moving cursor over matching fragments.
 * - Synchronized dual-column auto-scrolling to keep aligned fragments in view.
 */
class TextComparisonVisualizer : public ITextComparisonVisualizer {
public:
    TextComparisonVisualizer() = default;
    ~TextComparisonVisualizer() override = default;

    rst_code_e generate_html(const TextComparisonInput& input, std::string& out_html_content) const override;

private:
    static std::string escape_html(const std::string& str);
    static std::string format_timestamp(uint64_t ms);
};

#endif // TEXT_COMPARISON_VISUALIZER_HPP
