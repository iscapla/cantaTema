#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <fstream>

#include "speech_recognition/whisper_speech_recognition.hpp"
#include "speech_recognition/mocks/mock_whisper_engine_wrapper.hpp"
#include "primitives/tool_paths.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::StrictMock;

class WhisperConfigAndInitTest : public ::testing::Test {
protected:
    void SetUp() override {
        mock_engine = std::make_shared<StrictMock<MockWhisperEngineWrapper>>();
        whisper_recog = std::make_unique<WhisperSpeechRecognition>(nullptr, mock_engine);

        test_model_file = (std::filesystem::current_path() / "ggml-test-model.bin").string();
        std::ofstream ofs(test_model_file);
        ofs << "dummy model data";
        ofs.close();
    }

    void TearDown() override {
        if (std::filesystem::exists(test_model_file)) {
            std::filesystem::remove(test_model_file);
        }
    }

    std::shared_ptr<StrictMock<MockWhisperEngineWrapper>> mock_engine;
    std::unique_ptr<WhisperSpeechRecognition> whisper_recog;
    std::string test_model_file;
};

TEST_F(WhisperConfigAndInitTest, DefaultStateIsIdle) {
    EXPECT_EQ(whisper_recog->get_status(), ISpeechRecognition::speech_recognition_status_e::IDLE);
}

TEST_F(WhisperConfigAndInitTest, NonExistentModelReturnsFileNotFound) {
    ISpeechRecognition::speech_recognition_config_t config;
    config.model_name = "non_existent_model_99999.bin";

    rst_code_e res = whisper_recog->initialize(config);
    EXPECT_EQ(res, FILE_NOT_FOUND);
    EXPECT_EQ(whisper_recog->get_status(), ISpeechRecognition::speech_recognition_status_e::ERROR);
}

TEST_F(WhisperConfigAndInitTest, InitializeSuccessCpuMode) {
    whisper_context* dummy_ctx = reinterpret_cast<whisper_context*>(0x12345);

    EXPECT_CALL(*mock_engine, init_from_file_with_params(test_model_file, _))
        .WillOnce(Return(dummy_ctx));

    ISpeechRecognition::speech_recognition_config_t config;
    config.model_name = test_model_file;
    config.use_gpu = false;

    rst_code_e res = whisper_recog->initialize(config);
    EXPECT_EQ(res, RST_OK);
    EXPECT_EQ(whisper_recog->get_status(), ISpeechRecognition::speech_recognition_status_e::IDLE);

    EXPECT_CALL(*mock_engine, free_context(dummy_ctx)).Times(1);
    whisper_recog.reset();
}

TEST_F(WhisperConfigAndInitTest, InitializeGpuModeSuccess) {
    whisper_context* dummy_ctx = reinterpret_cast<whisper_context*>(0x54321);

    EXPECT_CALL(*mock_engine, init_from_file_with_params(test_model_file, true))
        .WillOnce(Return(dummy_ctx));

    ISpeechRecognition::speech_recognition_config_t config;
    config.model_name = test_model_file;
    config.use_gpu = true;

    rst_code_e res = whisper_recog->initialize(config);
    EXPECT_EQ(res, RST_OK);
    EXPECT_EQ(whisper_recog->get_status(), ISpeechRecognition::speech_recognition_status_e::IDLE);

    EXPECT_CALL(*mock_engine, free_context(dummy_ctx)).Times(1);
    whisper_recog.reset();
}

TEST_F(WhisperConfigAndInitTest, InitializeGpuModeFallbackToCpuWhenGpuFails) {
    whisper_context* dummy_ctx = reinterpret_cast<whisper_context*>(0xABCDE);

    EXPECT_CALL(*mock_engine, init_from_file_with_params(test_model_file, true))
        .WillOnce(Return(nullptr));

    EXPECT_CALL(*mock_engine, init_from_file_with_params(test_model_file, false))
        .WillOnce(Return(dummy_ctx));

    ISpeechRecognition::speech_recognition_config_t config;
    config.model_name = test_model_file;
    config.use_gpu = true;

    rst_code_e res = whisper_recog->initialize(config);
    EXPECT_EQ(res, RST_OK);
    EXPECT_EQ(whisper_recog->get_status(), ISpeechRecognition::speech_recognition_status_e::IDLE);

    EXPECT_CALL(*mock_engine, free_context(dummy_ctx)).Times(1);
    whisper_recog.reset();
}

TEST_F(WhisperConfigAndInitTest, InitializeFailsWhenBothGpuAndCpuReturnNull) {
    EXPECT_CALL(*mock_engine, init_from_file_with_params(test_model_file, _))
        .WillRepeatedly(Return(nullptr));

    ISpeechRecognition::speech_recognition_config_t config;
    config.model_name = test_model_file;
    config.use_gpu = false;

    rst_code_e res = whisper_recog->initialize(config);
    EXPECT_EQ(res, UNKNOWN);
    EXPECT_EQ(whisper_recog->get_status(), ISpeechRecognition::speech_recognition_status_e::ERROR);
}

TEST_F(WhisperConfigAndInitTest, ReinitializationFreesPreviousContext) {
    whisper_context* ctx1 = reinterpret_cast<whisper_context*>(0x111);
    whisper_context* ctx2 = reinterpret_cast<whisper_context*>(0x222);

    EXPECT_CALL(*mock_engine, init_from_file_with_params(test_model_file, _))
        .WillOnce(Return(ctx1));

    ISpeechRecognition::speech_recognition_config_t config;
    config.model_name = test_model_file;
    config.use_gpu = false;

    EXPECT_EQ(whisper_recog->initialize(config), RST_OK);

    EXPECT_CALL(*mock_engine, free_context(ctx1)).Times(1);
    EXPECT_CALL(*mock_engine, init_from_file_with_params(test_model_file, _))
        .WillOnce(Return(ctx2));

    EXPECT_EQ(whisper_recog->initialize(config), RST_OK);

    EXPECT_CALL(*mock_engine, free_context(ctx2)).Times(1);
    whisper_recog.reset();
}
