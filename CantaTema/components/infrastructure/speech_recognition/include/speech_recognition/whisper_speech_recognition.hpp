#ifndef WHISPER_SPEECH_RECOGNITION_HPP
#define WHISPER_SPEECH_RECOGNITION_HPP

#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include "speech_recognition/i_speech_recognition.hpp"
#include "sound_system/i_sound_system.hpp"

struct whisper_context;

class WhisperSpeechRecognition : public ISpeechRecognition {
public:
    explicit WhisperSpeechRecognition(std::shared_ptr<ISoundSystem> sound_system = nullptr);
    ~WhisperSpeechRecognition() override;

    rst_code_e initialize(const speech_recognition_config_t& config) override;
    rst_code_e submit_task(const std::string& audio_file_path) override;
    speech_recognition_status_e get_status() override;
    rst_code_e get_result(std::string& text_document_path) override;
    rst_code_e get_segments(std::vector<TranscriptSegment>& out_segments) override;

private:
    std::shared_ptr<ISoundSystem> m_soundSystem;
    speech_recognition_config_t m_config;
    whisper_context* m_whisperCtx{nullptr};
    std::vector<TranscriptSegment> m_segments;
    std::string m_resultTextPath;
    std::mutex m_mutex;

    std::vector<float> decode_audio_to_pcm(const std::string& audio_file_path);
};

#endif // WHISPER_SPEECH_RECOGNITION_HPP
