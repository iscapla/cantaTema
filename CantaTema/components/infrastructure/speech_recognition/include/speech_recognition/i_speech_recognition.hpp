#ifndef I_SPEECH_RECOGNITION_HPP
#define I_SPEECH_RECOGNITION_HPP

#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <functional>

#include "primitives/definitions.hpp"

/**
 * @brief Represents a single transcribed speech segment with timing and confidence data.
 */
struct TranscriptSegment {
    uint64_t start_time_ms{0};
    uint64_t end_time_ms{0};
    std::string text;
    float avg_logprob{0.0f};
    float confidence_score{1.0f};
};

class ISpeechRecognition {
public:
#ifdef ERROR
#undef ERROR
#endif
    enum class speech_recognition_status_e {
        IDLE,
        PROCESSING,
        COMPLETED,
        ERROR
    };

    using StatusCallback = std::function<void(speech_recognition_status_e)>;

    struct speech_recognition_config_t {
        std::string model_name;
        std::string language{"es"};
        StatusCallback status_callback;
        bool use_gpu{false};
    };

    virtual ~ISpeechRecognition() = default;

    virtual rst_code_e initialize(const speech_recognition_config_t& config) = 0;
    virtual rst_code_e submit_task(const std::string& audio_file_path) = 0;
    virtual speech_recognition_status_e get_status() = 0;
    virtual rst_code_e get_result(std::string& text_document_path) = 0;
    virtual rst_code_e get_segments(std::vector<TranscriptSegment>& out_segments) = 0;

protected:
    speech_recognition_status_e m_status{speech_recognition_status_e::IDLE};
};

#endif // I_SPEECH_RECOGNITION_HPP