#include <memory>
#include <string>
#include <functional>

#include "primitives/definitions.hpp"

class ISpeechRecognition {

public:
    enum class speech_recognition_status_e {
        IDLE,
        PROCESSING,
        COMPLETED,
        ERROR
    };

    using StatusCallback = std::function<void(speech_recognition_status_e)>;

    struct speech_recognition_config_t {
        std::string model_name;
        StatusCallback status_callback;
    };

    virtual ~ISpeechRecognition() = default;

    virtual rst_code_e initialize(const speech_recognition_config_t& config) = 0;
    virtual rst_code_e submit_task(const std::string& audio_file_path) = 0;
    virtual speech_recognition_status_e get_status() = 0;
    virtual rst_code_e get_result(std::string& text_document_path) = 0;

protected:
    speech_recognition_status_e m_status{speech_recognition_status_e::IDLE};
};