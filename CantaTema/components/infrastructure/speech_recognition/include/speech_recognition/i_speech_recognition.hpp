/**
 * @file i_speech_recognition.hpp
 * @brief Abstract interface for speech recognition engines and transcript segment data structures.
 */

#ifndef I_SPEECH_RECOGNITION_HPP
#define I_SPEECH_RECOGNITION_HPP

#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <functional>

#include "primitives/definitions.hpp"
#include "primitives/user_configuration.hpp"

/**
 * @struct TranscriptSegment
 * @brief Represents a single transcribed speech segment with timing, text, and confidence data.
 */
struct TranscriptSegment {
    uint64_t start_time_ms{0};     ///< Segment start timestamp in milliseconds.
    uint64_t end_time_ms{0};       ///< Segment end timestamp in milliseconds.
    std::string text;              ///< Transcribed spoken text.
    float avg_logprob{0.0f};       ///< Average token log-probability from Whisper model output.
    float confidence_score{1.0f};  ///< Normalized confidence rating (0.0 - 1.0).
    std::vector<size_t> source_segment_indices; ///< Raw acoustic segment indices merged into this segment.
};

/**
 * @class ISpeechRecognition
 * @brief Abstract interface defining speech recognition task lifecycle and transcription retrieval.
 */
class ISpeechRecognition {
public:
#ifdef ERROR
#undef ERROR
#endif
    /**
     * @enum speech_recognition_status_e
     * @brief State status levels of a speech recognition processing pipeline.
     */
    enum class speech_recognition_status_e {
        IDLE,        ///< Engine is ready to accept new tasks.
        PROCESSING,  ///< Inference pipeline is currently running.
        COMPLETED,   ///< Transcription task finished successfully.
        ERROR        ///< Task execution encountered an error.
    };

    /// Status callback function signature.
    using StatusCallback = std::function<void(speech_recognition_status_e)>;
    /// Progress callback function signature receiving percentage complete (0-100).
    using ProgressCallback = std::function<void(int progress_pct)>;

    /**
     * @struct speech_recognition_config_t
     * @brief Configuration parameters passed to initialize speech recognition tasks.
     */
    struct speech_recognition_config_t {
        std::string model_name;             ///< Whisper model file name or path.
        std::string language{"es"};         ///< Target spoken language ISO code.
        StatusCallback status_callback;     ///< Optional status change callback.
        ProgressCallback progress_callback; ///< Optional progress update callback.
        bool use_gpu{false};                ///< Whether GPU hardware acceleration should be enabled.
    };

    /**
     * @brief Virtual destructor for ISpeechRecognition.
     */
    virtual ~ISpeechRecognition() = default;

    /**
     * @brief Initializes speech recognition engine with task parameters.
     * @param config Configuration parameters struct.
     * @return rst_code_e RST_OK on success, or error code.
     */
    virtual rst_code_e initialize(const speech_recognition_config_t& config) = 0;

    /**
     * @brief Initializes speech recognition engine from UserConfiguration settings.
     * @param user_config User configuration snapshot.
     * @return rst_code_e RST_OK on success, or error code.
     */
    virtual rst_code_e initialize(const UserConfiguration& user_config) = 0;

    /**
     * @brief Submits an audio file path to start speech recognition transcription.
     * @param audio_file_path Path to input audio file.
     * @return rst_code_e RST_OK on success, or error code.
     */
    virtual rst_code_e submit_task(const std::string& audio_file_path) = 0;

    /**
     * @brief Queries current processing status.
     * @return speech_recognition_status_e Status enum value.
     */
    virtual speech_recognition_status_e get_status() = 0;

    /**
     * @brief Retrieves the filesystem path to saved transcript text result.
     * @param text_document_path Output string receiving text file path.
     * @return rst_code_e RST_OK on success, or error code.
     */
    virtual rst_code_e get_result(std::string& text_document_path) = 0;

    /**
     * @brief Retrieves detailed transcript segments with timing and confidence data.
     * @param out_segments Output vector receiving transcript segments.
     * @return rst_code_e RST_OK on success, or error code.
     */
    virtual rst_code_e get_segments(std::vector<TranscriptSegment>& out_segments) = 0;

protected:
    speech_recognition_status_e m_status{speech_recognition_status_e::IDLE};
};

#endif // I_SPEECH_RECOGNITION_HPP