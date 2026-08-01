#ifndef WHISPER_ACCURACY_VISUALIZER_HPP
#define WHISPER_ACCURACY_VISUALIZER_HPP

#include "reports/i_whisper_accuracy_visualizer.hpp"

/**
 * @class WhisperAccuracyVisualizer
 * @brief Concrete implementation of IWhisperAccuracyVisualizer.
 * Generates an interactive HTML page displaying Whisper transcription accuracy.
 */
class WhisperAccuracyVisualizer : public IWhisperAccuracyVisualizer {
public:
    WhisperAccuracyVisualizer() = default;
    ~WhisperAccuracyVisualizer() override = default;

    rst_code_e generate_html(const WhisperAccuracyInput& input, std::string& out_html_content) const override;

private:
    static std::string escape_html(const std::string& str);
    static std::string format_timestamp(uint64_t ms);
};

#endif // WHISPER_ACCURACY_VISUALIZER_HPP
