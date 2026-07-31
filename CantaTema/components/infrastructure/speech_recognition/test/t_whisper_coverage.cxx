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

class WhisperCoverageTest : public ::testing::Test {
protected:
    void create_valid_wav_file(const std::string& path, uint32_t sample_rate, uint16_t channels) {
        FILE* out = fopen(path.c_str(), "wb");
        if (!out) return;

        uint16_t bits_per_sample = 16;
        std::vector<int16_t> samples(sample_rate * 2, 0);

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

    void SetUp() override {
        mock_engine = std::make_shared<StrictMock<MockWhisperEngineWrapper>>();
        whisper_recog = std::make_unique<WhisperSpeechRecognition>(nullptr, mock_engine);

        test_dir = std::filesystem::current_path() / "test_whisper_coverage_scratch";
        std::filesystem::create_directories(test_dir);

        valid_wav_path = (test_dir / "valid.wav").string();
        create_valid_wav_file(valid_wav_path, 16000, 1);

        fake_model_file = (test_dir / "model.bin").string();
        std::ofstream ofs(fake_model_file);
        ofs << "dummy model data";
        ofs.close();

        dummy_ctx = reinterpret_cast<whisper_context*>(0x777);
        EXPECT_CALL(*mock_engine, init_from_file_with_params(fake_model_file, _))
            .WillOnce(Return(dummy_ctx));

        ISpeechRecognition::speech_recognition_config_t config;
        config.model_name = fake_model_file;
        config.language = "es";
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
    std::string valid_wav_path;
    std::string fake_model_file;
};

// 1. Audio conversion (whisper). wrong file
TEST_F(WhisperCoverageTest, AudioConversionWrongFile) {
    std::string missing_path = (test_dir / "non_existent_audio.wav").string();
    rst_code_e res = whisper_recog->submit_task(missing_path);
    EXPECT_EQ(res, FILE_NOT_FOUND);
    EXPECT_EQ(whisper_recog->get_status(), ISpeechRecognition::speech_recognition_status_e::ERROR);
}

// 2. Audio conversion (whisper). errors during conversion
TEST_F(WhisperCoverageTest, AudioConversionErrorsDuringConversion) {
    std::string corrupt_opus = (test_dir / "corrupt_input.opus").string();
    std::ofstream ofs(corrupt_opus, std::ios::binary);
    ofs << "RANDOM_CORRUPT_NON_OPUS_GARBAGE_BYTES_123456789";
    ofs.close();

    rst_code_e res = whisper_recog->submit_task(corrupt_opus);
    EXPECT_NE(res, RST_OK);
    EXPECT_EQ(whisper_recog->get_status(), ISpeechRecognition::speech_recognition_status_e::ERROR);
}

// 3. Audio conversion (whisper). full accurate conversion
TEST_F(WhisperCoverageTest, AudioConversionFullAccurateConversion) {
    EXPECT_CALL(*mock_engine, run_full(dummy_ctx, "es", _)).WillOnce(Return(0));

    std::vector<WhisperSegmentData> mock_segs = {
        {0, 2000, "Transcripcion perfecta de alta precision.", 0.98f, -0.02f}
    };
    EXPECT_CALL(*mock_engine, extract_segments(dummy_ctx)).WillOnce(Return(mock_segs));

    EXPECT_EQ(whisper_recog->submit_task(valid_wav_path), RST_OK);
    EXPECT_EQ(whisper_recog->get_status(), ISpeechRecognition::speech_recognition_status_e::COMPLETED);

    std::vector<TranscriptSegment> segments;
    EXPECT_EQ(whisper_recog->get_segments(segments), RST_OK);
    ASSERT_EQ(segments.size(), 1u);
    EXPECT_EQ(segments[0].text, "Transcripcion perfecta de alta precision.");
    EXPECT_FLOAT_EQ(segments[0].confidence_score, 0.98f);
}

// 4. Audio conversion (whisper). less accurate conversion
TEST_F(WhisperCoverageTest, AudioConversionLessAccurateConversion) {
    EXPECT_CALL(*mock_engine, run_full(dummy_ctx, "es", _)).WillOnce(Return(0));

    std::vector<WhisperSegmentData> mock_segs = {
        {0, 2000, "Texto dudoso o ruidoso.", 0.35f, -1.25f}
    };
    EXPECT_CALL(*mock_engine, extract_segments(dummy_ctx)).WillOnce(Return(mock_segs));

    EXPECT_EQ(whisper_recog->submit_task(valid_wav_path), RST_OK);
    EXPECT_EQ(whisper_recog->get_status(), ISpeechRecognition::speech_recognition_status_e::COMPLETED);

    std::vector<TranscriptSegment> segments;
    EXPECT_EQ(whisper_recog->get_segments(segments), RST_OK);
    ASSERT_EQ(segments.size(), 1u);
    EXPECT_FLOAT_EQ(segments[0].confidence_score, 0.35f);
}

// 5. Audio conversion (whisper). Different language usage
TEST_F(WhisperCoverageTest, AudioConversionDifferentLanguageUsage) {
    whisper_context* en_ctx = reinterpret_cast<whisper_context*>(0x888);

    EXPECT_CALL(*mock_engine, free_context(dummy_ctx)).Times(1);
    dummy_ctx = nullptr; // Prevent double free in TearDown

    EXPECT_CALL(*mock_engine, init_from_file_with_params(fake_model_file, _))
        .WillOnce(Return(en_ctx));

    EXPECT_CALL(*mock_engine, run_full(en_ctx, "en", _)).WillOnce(Return(0));

    std::vector<WhisperSegmentData> mock_segs = {
        {0, 2000, "High precision English transcription.", 0.95f, -0.05f}
    };
    EXPECT_CALL(*mock_engine, extract_segments(en_ctx)).WillOnce(Return(mock_segs));

    EXPECT_CALL(*mock_engine, free_context(en_ctx)).Times(1);

    // Re-initialize with English configuration
    ISpeechRecognition::speech_recognition_config_t config;
    config.model_name = fake_model_file;
    config.language = "en";
    config.use_gpu = false;
    whisper_recog->initialize(config);

    EXPECT_EQ(whisper_recog->submit_task(valid_wav_path), RST_OK);
    EXPECT_EQ(whisper_recog->get_status(), ISpeechRecognition::speech_recognition_status_e::COMPLETED);

    std::vector<TranscriptSegment> segments;
    EXPECT_EQ(whisper_recog->get_segments(segments), RST_OK);
    ASSERT_EQ(segments.size(), 1u);
    EXPECT_EQ(segments[0].text, "High precision English transcription.");
}
