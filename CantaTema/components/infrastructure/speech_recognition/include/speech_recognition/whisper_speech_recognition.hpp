/**
 * @file whisper_speech_recognition.hpp
 * @brief Concrete implementation of ISpeechRecognition wrapping Whisper.cpp engine.
 */

#ifndef WHISPER_SPEECH_RECOGNITION_HPP
#define WHISPER_SPEECH_RECOGNITION_HPP

#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include "speech_recognition/i_speech_recognition.hpp"
#include "sound_system/i_sound_system.hpp"

struct whisper_context;

namespace cantatema::infra {
class IGpuDetector;
}

class IWhisperEngineWrapper;

/**
 * @class WhisperSpeechRecognition
 * @brief Concrete speech recognition implementation leveraging local Whisper.cpp inference.
 */
class WhisperSpeechRecognition : public ISpeechRecognition {
public:
    /**
     * @brief Constructs a WhisperSpeechRecognition instance with optional dependency injection.
     * @param sound_system Pointer to sound system interface for audio decoding.
     * @param engine_wrapper Pointer to Whisper engine wrapper for model execution.
     * @param gpu_detector Pointer to GPU detector for acceleration strategy evaluation.
     */
    explicit WhisperSpeechRecognition(
        std::shared_ptr<ISoundSystem> sound_system = nullptr,
        std::shared_ptr<IWhisperEngineWrapper> engine_wrapper = nullptr,
        std::shared_ptr<cantatema::infra::IGpuDetector> gpu_detector = nullptr
    );

    /**
     * @brief Destructor releasing active whisper contexts and temporary audio resources.
     */
    ~WhisperSpeechRecognition() override;

    /**
     * @brief Initializes speech recognition with explicit configuration parameters.
     * @param config Parameters specifying model name, language, GPU usage, and callbacks.
     * @return rst_code_e RST_OK on successful initialization, or error code.
     */
    rst_code_e initialize(const speech_recognition_config_t& config) override;

    /**
     * @brief Initializes speech recognition using unified UserConfiguration parameters.
     * @param user_config User configuration snapshot.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e initialize(const UserConfiguration& user_config) override;

    /**
     * @brief Submits an audio file for speech-to-text transcription.
     * @param audio_file_path Path to the input audio file (.opus or .wav).
     * @return rst_code_e RST_OK on successful execution, or error code.
     */
    rst_code_e submit_task(const std::string& audio_file_path) override;

    /**
     * @brief Gets current execution status of speech recognition pipeline.
     * @return speech_recognition_status_e Current status enum.
     */
    speech_recognition_status_e get_status() override;

    /**
     * @brief Retrieves the output text document filepath containing full transcription text.
     * @param text_document_path Output string receiving the path to generated text file.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e get_result(std::string& text_document_path) override;

    /**
     * @brief Gets individual transcribed text segments with timecodes and confidence scores.
     * @param out_segments Output vector receiving transcript segment records.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e get_segments(std::vector<TranscriptSegment>& out_segments) override;

private:
    std::shared_ptr<ISoundSystem> m_soundSystem;
    std::shared_ptr<IWhisperEngineWrapper> m_engineWrapper;
    std::shared_ptr<cantatema::infra::IGpuDetector> m_gpuDetector;
    speech_recognition_config_t m_config;
    whisper_context* m_whisperCtx{nullptr};
    std::vector<TranscriptSegment> m_segments;
    std::string m_resultTextPath;
    std::mutex m_mutex;

    /**
     * @brief Converts and resamples input audio file into 16kHz mono float PCM samples expected by Whisper.
     * @param audio_file_path Path to input audio file.
     * @return std::vector<float> Vector of normalized PCM audio samples [-1.0, 1.0].
     */
    std::vector<float> decode_audio_to_pcm(const std::string& audio_file_path);
};

#endif // WHISPER_SPEECH_RECOGNITION_HPP
