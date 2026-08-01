/**
 * @file whisper_accuracy_visualizer.hpp
 * @brief Concrete HTML visualizer for Whisper speech transcription accuracy and confidence analysis.
 */

#ifndef WHISPER_ACCURACY_VISUALIZER_HPP
#define WHISPER_ACCURACY_VISUALIZER_HPP

#include "reports/i_whisper_accuracy_visualizer.hpp"

/**
 * @class WhisperAccuracyVisualizer
 * @brief Concrete implementation of IWhisperAccuracyVisualizer.
 *
 * Renders an interactive HTML document visualizing Whisper speech-to-text accuracy.
 * Color-codes transcription segments according to confidence thresholds (Green/Yellow/Red)
 * and formats metadata headers including speech rate (WPM), clarity, and audio metrics.
 */
class WhisperAccuracyVisualizer : public IWhisperAccuracyVisualizer {
public:
    /**
     * @brief Constructs a WhisperAccuracyVisualizer object.
     */
    WhisperAccuracyVisualizer() = default;

    /**
     * @brief Destroys the WhisperAccuracyVisualizer object.
     */
    ~WhisperAccuracyVisualizer() override = default;

    /**
     * @brief Generates an HTML report displaying color-coded transcript segments and performance metadata.
     * @param input Data payload containing audio metadata, metrics, and transcript segments.
     * @param out_html_content Output string buffer where generated HTML content will be stored.
     * @return rst_code_e RST_OK on success, or appropriate error code on failure.
     */
    rst_code_e generate_html(const WhisperAccuracyInput& input, std::string& out_html_content) const override;

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

#endif // WHISPER_ACCURACY_VISUALIZER_HPP
