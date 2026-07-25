#ifndef I_WHISPER_ENGINE_WRAPPER_HPP
#define I_WHISPER_ENGINE_WRAPPER_HPP

#include <string>
#include <vector>
#include <cstdint>

struct whisper_context;

/**
 * @struct WhisperSegmentData
 * @brief Structured representation of a decoded Whisper segment.
 */
struct WhisperSegmentData {
    int64_t t0{0};
    int64_t t1{0};
    std::string text;
    float avg_confidence{1.0f};
    float avg_logprob{0.0f};
};

/**
 * @class IWhisperEngineWrapper
 * @brief Interface wrapping low-level whisper.cpp C library calls for dependency injection and mocking.
 */
class IWhisperEngineWrapper {
public:
    virtual ~IWhisperEngineWrapper() = default;

    /**
     * @brief Initialize a whisper context from file with specified parameters.
     * @param model_path Path to the binary model file.
     * @param use_gpu Whether GPU acceleration is requested.
     * @return Pointer to opaque whisper context, or nullptr on failure.
     */
    virtual whisper_context* init_from_file_with_params(const std::string& model_path, bool use_gpu) = 0;

    /**
     * @brief Free an active whisper context.
     * @param ctx Pointer to opaque whisper context to free.
     */
    virtual void free_context(whisper_context* ctx) = 0;

    /**
     * @brief Execute full whisper transcription on PCM float samples.
     * @param ctx Pointer to opaque whisper context.
     * @param language Target language string (e.g. "es").
     * @param pcm_samples Floating-point PCM audio samples at 16kHz.
     * @return 0 on success, or non-zero error code.
     */
    virtual int run_full(whisper_context* ctx, const std::string& language, const std::vector<float>& pcm_samples) = 0;

    /**
     * @brief Extract decoded segments and confidence metrics from a completed whisper context.
     * @param ctx Pointer to opaque whisper context.
     * @return Vector of WhisperSegmentData structures.
     */
    virtual std::vector<WhisperSegmentData> extract_segments(whisper_context* ctx) = 0;
};

#endif // I_WHISPER_ENGINE_WRAPPER_HPP
