#ifndef MOCK_WHISPER_ENGINE_WRAPPER_HPP
#define MOCK_WHISPER_ENGINE_WRAPPER_HPP

#include <gmock/gmock.h>
#include "speech_recognition/i_whisper_engine_wrapper.hpp"

class MockWhisperEngineWrapper : public IWhisperEngineWrapper {
public:
    MockWhisperEngineWrapper() = default;
    ~MockWhisperEngineWrapper() override = default;

    MOCK_METHOD(whisper_context*, init_from_file_with_params, (const std::string& model_path, bool use_gpu), (override));
    MOCK_METHOD(void, free_context, (whisper_context* ctx), (override));
    MOCK_METHOD(int, run_full, (whisper_context* ctx, const std::string& language, const std::vector<float>& pcm_samples), (override));
    MOCK_METHOD(std::vector<WhisperSegmentData>, extract_segments, (whisper_context* ctx), (override));
};

#endif // MOCK_WHISPER_ENGINE_WRAPPER_HPP
