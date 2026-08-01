/**
 * @file i_whisper_accuracy_visualizer.hpp
 * @brief Abstract interface and data contracts for generating Whisper transcription accuracy visualizer reports.
 */

#ifndef I_WHISPER_ACCURACY_VISUALIZER_HPP
#define I_WHISPER_ACCURACY_VISUALIZER_HPP

#include <string>
#include <vector>
#include <cstdint>
#include "primitives/definitions.hpp"
#include "speech_recognition/i_speech_recognition.hpp"

/**
 * @struct WhisperAccuracyConfig
 * @brief Threshold parameters for color-coding transcription confidence.
 */
struct WhisperAccuracyConfig {
    float high_confidence_threshold{0.85f};   ///< Threshold for high confidence (Level 1: Green)
    float medium_confidence_threshold{0.60f}; ///< Threshold for medium confidence (Level 2: Yellow)
};

/**
 * @struct WhisperAccuracyInput
 * @brief Input payload containing transcript segments and audio conversion metadata.
 */
struct WhisperAccuracyInput {
    std::string audio_filepath;               ///< Absolute or relative filesystem path to input audio file.
    std::string model_name;                   ///< Name/identifier of the loaded Whisper model (e.g. ggml-small.bin).
    std::string language;                     ///< ISO language code for speech recognition (e.g. "es").
    uint64_t total_duration_ms{0};            ///< Total duration of processed audio in milliseconds.
    uint64_t processing_time_ms{0};           ///< Total inference duration in milliseconds.
    float speech_rate_wpm{0.0f};              ///< Average speaking velocity in Words Per Minute.
    float clarity_score{0.0f};                ///< Normalized clarity score derived from token probabilities (0-100).
    float overall_confidence{0.0f};           ///< Average overall confidence percentage across all transcript segments.
    std::vector<TranscriptSegment> segments;  ///< Array of transcribed text segments with timecodes and token scores.
    WhisperAccuracyConfig config;             ///< Threshold configuration parameters for visual rendering.
};

/**
 * @class IWhisperAccuracyVisualizer
 * @brief Abstract interface for generating HTML visualization of Whisper transcription accuracy.
 */
class IWhisperAccuracyVisualizer {
public:
    /**
     * @brief Virtual destructor for IWhisperAccuracyVisualizer.
     */
    virtual ~IWhisperAccuracyVisualizer() = default;

    /**
     * @brief Generates an HTML report string displaying color-coded transcript text and metadata.
     * @param input Input structure with transcription segments and metadata.
     * @param out_html_content Output string receiving the full generated HTML document.
     * @return rst_code_e RST_OK on success, or error code.
     */
    virtual rst_code_e generate_html(const WhisperAccuracyInput& input, std::string& out_html_content) const = 0;
};

#endif // I_WHISPER_ACCURACY_VISUALIZER_HPP
