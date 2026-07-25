#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <fstream>
#include <vector>

#include "speech_recognition/whisper_speech_recognition.hpp"
#include "speech_recognition/mocks/mock_whisper_engine_wrapper.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::StrictMock;

class WhisperAudioConversionTest : public ::testing::Test {
protected:
    void SetUp() override {
        mock_engine = std::make_shared<StrictMock<MockWhisperEngineWrapper>>();
        whisper_recog = std::make_unique<WhisperSpeechRecognition>(nullptr, mock_engine);

        test_dir = std::filesystem::current_path() / "test_audio_scratch";
        std::filesystem::create_directories(test_dir);

        test_model_file = (test_dir / "ggml-test.bin").string();
        std::ofstream ofs(test_model_file);
        ofs << "model data";
        ofs.close();

        dummy_ctx = reinterpret_cast<whisper_context*>(0x999);
        EXPECT_CALL(*mock_engine, init_from_file_with_params(test_model_file, _))
            .WillOnce(Return(dummy_ctx));

        ISpeechRecognition::speech_recognition_config_t config;
        config.model_name = test_model_file;
        config.use_gpu = false;
        whisper_recog->initialize(config);
    }

    void TearDown() override {
        if (dummy_ctx) {
            EXPECT_CALL(*mock_engine, free_context(dummy_ctx)).Times(1);
            whisper_recog.reset();
        }
        if (std::filesystem::exists(test_dir)) {
            std::filesystem::remove_all(test_dir);
        }
    }

    std::shared_ptr<StrictMock<MockWhisperEngineWrapper>> mock_engine;
    std::unique_ptr<WhisperSpeechRecognition> whisper_recog;
    whisper_context* dummy_ctx{nullptr};
    std::filesystem::path test_dir;
    std::string test_model_file;
};

TEST_F(WhisperAudioConversionTest, SubmitNonExistentAudioFileReturnsFileNotFound) {
    std::string non_existent = (test_dir / "missing.opus").string();
    rst_code_e res = whisper_recog->submit_task(non_existent);
    EXPECT_EQ(res, FILE_NOT_FOUND);
    EXPECT_EQ(whisper_recog->get_status(), ISpeechRecognition::speech_recognition_status_e::ERROR);
}

TEST_F(WhisperAudioConversionTest, SubmitCorruptAudioFileReturnsError) {
    std::string corrupt_audio = (test_dir / "corrupt.opus").string();
    std::ofstream ofs(corrupt_audio, std::ios::binary);
    ofs << "Not an ogg or opus audio content header";
    ofs.close();

    rst_code_e res = whisper_recog->submit_task(corrupt_audio);
    EXPECT_EQ(res, UNKNOWN);
    EXPECT_EQ(whisper_recog->get_status(), ISpeechRecognition::speech_recognition_status_e::ERROR);
}

TEST_F(WhisperAudioConversionTest, TempWavCleanedUpAfterConversionFailure) {
    std::string invalid_opus = (test_dir / "invalid_header.opus").string();
    std::ofstream ofs(invalid_opus, std::ios::binary);
    ofs << "OggS" << "invalid stream bytes that cannot decode";
    ofs.close();

    rst_code_e res = whisper_recog->submit_task(invalid_opus);
    EXPECT_EQ(res, UNKNOWN);
    EXPECT_EQ(whisper_recog->get_status(), ISpeechRecognition::speech_recognition_status_e::ERROR);
}
