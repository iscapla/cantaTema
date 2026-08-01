/**
 * @file whisper_engine_wrapper.hpp
 * @brief Concrete wrapper for whisper.cpp C engine API calls.
 */

#ifndef WHISPER_ENGINE_WRAPPER_HPP
#define WHISPER_ENGINE_WRAPPER_HPP

#include "speech_recognition/i_whisper_engine_wrapper.hpp"

/**
 * @class WhisperEngineWrapper
 * @brief Concrete implementation of IWhisperEngineWrapper delegating directly to whisper.cpp C API.
 */
class WhisperEngineWrapper : public IWhisperEngineWrapper {
public:
    /**
     * @brief Constructs a WhisperEngineWrapper instance.
     */
    WhisperEngineWrapper() = default;

    /**
     * @brief Destructor for WhisperEngineWrapper.
     */
    ~WhisperEngineWrapper() override = default;

    /**
     * @brief Initializes a whisper_context from a model binary file with optional GPU acceleration.
     * @param model_path Path to the Whisper .bin model file.
     * @param use_gpu Whether GPU offload is requested.
     * @return whisper_context* Opaque pointer to initialized context, or nullptr on error.
     */
    whisper_context* init_from_file_with_params(const std::string& model_path, bool use_gpu) override;

    /**
     * @brief Frees an active whisper_context instance.
     * @param ctx Pointer to the whisper_context to free.
     */
    void free_context(whisper_context* ctx) override;

    /**
     * @brief Runs full Whisper decoding on PCM audio samples without progress callback.
     * @param ctx Active whisper_context pointer.
     * @param language Target language ISO string (e.g. "es").
     * @param pcm_samples Vector of 16kHz mono float PCM samples.
     * @return int 0 on success, or non-zero error code.
     */
    int run_full(whisper_context* ctx, const std::string& language, const std::vector<float>& pcm_samples) override;

    /**
     * @brief Runs full Whisper decoding on PCM audio samples with progress callback notifications.
     * @param ctx Active whisper_context pointer.
     * @param language Target language ISO string (e.g. "es").
     * @param pcm_samples Vector of 16kHz mono float PCM samples.
     * @param progress_cb Callback receiving percentage progress (0-100).
     * @return int 0 on success, or non-zero error code.
     */
    int run_full(whisper_context* ctx, const std::string& language, const std::vector<float>& pcm_samples, std::function<void(int)> progress_cb) override;

    /**
     * @brief Extracts segment timestamps, confidence levels, and text from completed whisper decoding.
     * @param ctx Active whisper_context pointer.
     * @return std::vector<WhisperSegmentData> List of extracted segment records.
     */
    std::vector<WhisperSegmentData> extract_segments(whisper_context* ctx) override;
};

#endif // WHISPER_ENGINE_WRAPPER_HPP
