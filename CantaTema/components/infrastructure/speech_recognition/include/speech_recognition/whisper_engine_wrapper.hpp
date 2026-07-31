#ifndef WHISPER_ENGINE_WRAPPER_HPP
#define WHISPER_ENGINE_WRAPPER_HPP

#include "speech_recognition/i_whisper_engine_wrapper.hpp"

/**
 * @class WhisperEngineWrapper
 * @brief Concrete implementation of IWhisperEngineWrapper delegating directly to whisper.cpp C API.
 */
class WhisperEngineWrapper : public IWhisperEngineWrapper {
public:
    WhisperEngineWrapper() = default;
    ~WhisperEngineWrapper() override = default;

    whisper_context* init_from_file_with_params(const std::string& model_path, bool use_gpu) override;
    void free_context(whisper_context* ctx) override;
    int run_full(whisper_context* ctx, const std::string& language, const std::vector<float>& pcm_samples) override;
    int run_full(whisper_context* ctx, const std::string& language, const std::vector<float>& pcm_samples, std::function<void(int)> progress_cb) override;
    std::vector<WhisperSegmentData> extract_segments(whisper_context* ctx) override;
};

#endif // WHISPER_ENGINE_WRAPPER_HPP
