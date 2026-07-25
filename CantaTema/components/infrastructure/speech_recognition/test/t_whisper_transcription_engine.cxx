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

class WhisperTranscriptionEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        mock_engine = std::make_shared<StrictMock<MockWhisperEngineWrapper>>();
        whisper_recog = std::make_unique<WhisperSpeechRecognition>(nullptr, mock_engine);

        test_dir = std::filesystem::current_path() / "test_engine_scratch";
        std::filesystem::create_directories(test_dir);

        test_model_file = (test_dir / "ggml-test.bin").string();
        std::ofstream ofs(test_model_file);
        ofs << "model data";
        ofs.close();

        // Standard 16kHz mono WAV file for PCM decoding test
        test_wav_file = (test_dir / "valid_test.wav").string();
        create_test_wav_file(test_wav_file, 16000, 1);
    }

    void TearDown() override {
        if (std::filesystem::exists(test_dir)) {
            std::filesystem::remove_all(test_dir);
        }
    }

    void create_test_wav_file(const std::string& path, uint32_t sample_rate, uint16_t channels) {
        FILE* out = fopen(path.c_str(), "wb");
        if (!out) return;

        uint16_t bits_per_sample = 16;
        std::vector<int16_t> samples(sample_rate, 0); // 1 second of silence PCM

        uint32_t data_size = static_cast<uint32_t>(samples.size() * sizeof(int16_t));
        uint32_t chunk_size = 36 + data_size;
        uint32_t subchunk1_size = 16;
        uint16_t audio_format = 1;
        uint32_t byte_rate = sample_rate * channels * (bits_per_sample / 8);
        uint16_t block_align = channels * (bits_per_sample / 8);

        fwrite("RIFF", 1, 4, out);
        fwrite(&chunk_size, 4, 1, out);
        fwrite("WAVE", 1, 4, out);
        fwrite("fmt ", 1, 4, out);
        fwrite(&subchunk1_size, 4, 1, out);
        fwrite(&audio_format, 2, 1, out);
        fwrite(&channels, 2, 1, out);
        fwrite(&sample_rate, 4, 1, out);
        fwrite(&byte_rate, 4, 1, out);
        fwrite(&block_align, 2, 1, out);
        fwrite(&bits_per_sample, 2, 1, out);
        fwrite("data", 1, 4, out);
        fwrite(&data_size, 4, 1, out);
        fwrite(samples.data(), sizeof(int16_t), samples.size(), out);
        fclose(out);
    }

    std::shared_ptr<StrictMock<MockWhisperEngineWrapper>> mock_engine;
    std::unique_ptr<WhisperSpeechRecognition> whisper_recog;
    std::filesystem::path test_dir;
    std::string test_model_file;
    std::string test_wav_file;
};

TEST_F(WhisperTranscriptionEngineTest, SubmitTaskUninitializedReturnsError) {
    rst_code_e res = whisper_recog->submit_task(test_wav_file);
    EXPECT_EQ(res, UNKNOWN);
    EXPECT_EQ(whisper_recog->get_status(), ISpeechRecognition::speech_recognition_status_e::ERROR);
}

TEST_F(WhisperTranscriptionEngineTest, SubmitTaskTranscriptionSuccess) {
    whisper_context* dummy_ctx = reinterpret_cast<whisper_context*>(0x777);
    EXPECT_CALL(*mock_engine, init_from_file_with_params(test_model_file, _))
        .WillOnce(Return(dummy_ctx));

    std::vector<ISpeechRecognition::speech_recognition_status_e> status_updates;

    ISpeechRecognition::speech_recognition_config_t config;
    config.model_name = test_model_file;
    config.language = "es";
    config.use_gpu = false;
    config.status_callback = [&](ISpeechRecognition::speech_recognition_status_e st) {
        status_updates.push_back(st);
    };

    EXPECT_EQ(whisper_recog->initialize(config), RST_OK);

    EXPECT_CALL(*mock_engine, run_full(dummy_ctx, "es", _))
        .WillOnce(Return(0));

    std::vector<WhisperSegmentData> mock_segments = {
        {0, 2000, "Hola a todos", 0.95f, -0.05f},
        {2100, 4500, "Esta es una prueba de transcripcion", 0.90f, -0.10f}
    };

    EXPECT_CALL(*mock_engine, extract_segments(dummy_ctx))
        .WillOnce(Return(mock_segments));

    rst_code_e res = whisper_recog->submit_task(test_wav_file);
    EXPECT_EQ(res, RST_OK);
    EXPECT_EQ(whisper_recog->get_status(), ISpeechRecognition::speech_recognition_status_e::COMPLETED);

    // Verify status callback sequence
    ASSERT_GE(status_updates.size(), 2u);
    EXPECT_EQ(status_updates[0], ISpeechRecognition::speech_recognition_status_e::PROCESSING);
    EXPECT_EQ(status_updates.back(), ISpeechRecognition::speech_recognition_status_e::COMPLETED);

    // Verify extracted segments
    std::vector<TranscriptSegment> segs;
    EXPECT_EQ(whisper_recog->get_segments(segs), RST_OK);
    ASSERT_EQ(segs.size(), 2u);
    EXPECT_EQ(segs[0].text, "Hola a todos");
    EXPECT_EQ(segs[0].start_time_ms, 0u);
    EXPECT_EQ(segs[0].end_time_ms, 2000u);
    EXPECT_FLOAT_EQ(segs[0].confidence_score, 0.95f);

    EXPECT_EQ(segs[1].text, "Esta es una prueba de transcripcion");
    EXPECT_EQ(segs[1].start_time_ms, 2100u);
    EXPECT_EQ(segs[1].end_time_ms, 4500u);

    // Verify get_result writes text file
    std::string text_file_out;
    EXPECT_EQ(whisper_recog->get_result(text_file_out), RST_OK);
    EXPECT_TRUE(std::filesystem::exists(text_file_out));

    EXPECT_CALL(*mock_engine, free_context(dummy_ctx)).Times(1);
    whisper_recog.reset();
}

TEST_F(WhisperTranscriptionEngineTest, SubmitTaskRunFullFailureReturnsError) {
    whisper_context* dummy_ctx = reinterpret_cast<whisper_context*>(0x888);
    EXPECT_CALL(*mock_engine, init_from_file_with_params(test_model_file, _))
        .WillOnce(Return(dummy_ctx));

    ISpeechRecognition::speech_recognition_config_t config;
    config.model_name = test_model_file;
    config.language = "es";
    config.use_gpu = false;

    EXPECT_EQ(whisper_recog->initialize(config), RST_OK);

    // run_full fails with error code -1
    EXPECT_CALL(*mock_engine, run_full(dummy_ctx, "es", _))
        .WillOnce(Return(-1));

    rst_code_e res = whisper_recog->submit_task(test_wav_file);
    EXPECT_EQ(res, UNKNOWN);
    EXPECT_EQ(whisper_recog->get_status(), ISpeechRecognition::speech_recognition_status_e::ERROR);

    EXPECT_CALL(*mock_engine, free_context(dummy_ctx)).Times(1);
    whisper_recog.reset();
}
