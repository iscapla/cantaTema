#include <gtest/gtest.h>
#include "speech_recognition/whisper_speech_recognition.hpp"

TEST(WhisperSpeechRecognitionTest, ConstructionAndDefaults) {
    WhisperSpeechRecognition whisper_recog;
    EXPECT_EQ(whisper_recog.get_status(), ISpeechRecognition::speech_recognition_status_e::IDLE);
}

TEST(WhisperSpeechRecognitionTest, InitializeNonExistentModelReturnsError) {
    WhisperSpeechRecognition whisper_recog;
    ISpeechRecognition::speech_recognition_config_t config;
    config.model_name = "non_existent_ggml_model_999.bin";
    config.language = "es";

    rst_code_e res = whisper_recog.initialize(config);
    EXPECT_EQ(res, FILE_NOT_FOUND);
    EXPECT_EQ(whisper_recog.get_status(), ISpeechRecognition::speech_recognition_status_e::ERROR);
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
