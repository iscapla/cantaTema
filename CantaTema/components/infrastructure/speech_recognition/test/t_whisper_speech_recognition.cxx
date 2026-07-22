#include <gtest/gtest.h>
#include <filesystem>
#include "speech_recognition/whisper_speech_recognition.hpp"
#include "primitives/tool_paths.hpp"

TEST(WhisperSpeechRecognitionTest, ConstructionAndDefaults) {
    WhisperSpeechRecognition whisper_recog;
    EXPECT_EQ(whisper_recog.get_status(), ISpeechRecognition::speech_recognition_status_e::IDLE);
}

TEST(WhisperSpeechRecognitionTest, ConfigDefaultsToCpuMode) {
    ISpeechRecognition::speech_recognition_config_t config;
    EXPECT_FALSE(config.use_gpu); // Must be CPU-first by default
}

TEST(WhisperSpeechRecognitionTest, InitializeNonExistentModelReturnsError) {
    WhisperSpeechRecognition whisper_recog;
    ISpeechRecognition::speech_recognition_config_t config;
    config.model_name = "non_existent_ggml_model_999.bin";
    config.language = "es";
    config.use_gpu = false; // CPU mode

    rst_code_e res = whisper_recog.initialize(config);
    EXPECT_EQ(res, FILE_NOT_FOUND);
    EXPECT_EQ(whisper_recog.get_status(), ISpeechRecognition::speech_recognition_status_e::ERROR);
}

TEST(WhisperSpeechRecognitionTest, InitializeCpuModeWithModelIfExists) {
    WhisperSpeechRecognition whisper_recog;
    std::filesystem::path model_dir = ToolPath::get_path_for_models_whisper();
    std::filesystem::path tiny_model = model_dir / "ggml-tiny.bin";

    if (std::filesystem::exists(tiny_model)) {
        ISpeechRecognition::speech_recognition_config_t config;
        config.model_name = "ggml-tiny.bin";
        config.language = "es";
        config.use_gpu = false; // Explicit CPU mode

        rst_code_e res = whisper_recog.initialize(config);
        EXPECT_EQ(res, RST_OK);
        EXPECT_EQ(whisper_recog.get_status(), ISpeechRecognition::speech_recognition_status_e::IDLE);
    }
}

TEST(WhisperSpeechRecognitionTest, InitializeGpuModeFallbackWithModelIfExists) {
    WhisperSpeechRecognition whisper_recog;
    std::filesystem::path model_dir = ToolPath::get_path_for_models_whisper();
    std::filesystem::path tiny_model = model_dir / "ggml-tiny.bin";

    if (std::filesystem::exists(tiny_model)) {
        ISpeechRecognition::speech_recognition_config_t config;
        config.model_name = "ggml-tiny.bin";
        config.language = "es";
        config.use_gpu = true; // Request GPU, test fallback if GPU is unavailable

        rst_code_e res = whisper_recog.initialize(config);
        // Initialization should either succeed on GPU or safely fall back to CPU without crashing
        EXPECT_EQ(res, RST_OK);
        EXPECT_EQ(whisper_recog.get_status(), ISpeechRecognition::speech_recognition_status_e::IDLE);
    }
}

TEST(WhisperSpeechRecognitionTest, SubmitTaskUninitializedReturnsError) {
    WhisperSpeechRecognition whisper_recog;
    rst_code_e res = whisper_recog.submit_task("dummy_audio.opus");
    EXPECT_EQ(res, UNKNOWN);
}

TEST(WhisperSpeechRecognitionTest, GetResultWhenNotCompleted) {
    WhisperSpeechRecognition whisper_recog;
    std::string out_path;
    rst_code_e res = whisper_recog.get_result(out_path);
    EXPECT_EQ(res, UNKNOWN);
}

TEST(WhisperSpeechRecognitionTest, GetSegmentsEmptyInitially) {
    WhisperSpeechRecognition whisper_recog;
    std::vector<TranscriptSegment> segments;
    rst_code_e res = whisper_recog.get_segments(segments);
    EXPECT_EQ(res, RST_OK);
    EXPECT_TRUE(segments.empty());
}

