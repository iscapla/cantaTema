#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <fstream>
#include <vector>

#include "speech_recognition/whisper_speech_recognition.hpp"
#include "speech_recognition/mocks/mock_whisper_engine_wrapper.hpp"
#include "primitives/tool_paths.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::StrictMock;

class WhisperQuijoteIntegrationTest : public ::testing::Test {
protected:
    void create_valid_wav_file(const std::string& path, uint32_t sample_rate, uint16_t channels) {
        FILE* out = fopen(path.c_str(), "wb");
        if (!out) return;

        uint16_t bits_per_sample = 16;
        std::vector<int16_t> samples(sample_rate * 5, 0); // 5 seconds of silence PCM

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

        std::filesystem::path base = ToolPath::get_base_path();
        quijote_wav_path = (base / "example_data" / "Don_Quijote_de_la_Mancha.wav").string();

        if (!std::filesystem::exists(quijote_wav_path)) {
            quijote_wav_path = (std::filesystem::current_path() / "Don_Quijote_de_la_Mancha.wav").string();
            create_valid_wav_file(quijote_wav_path, 16000, 1);
            created_fallback_wav = true;
        }

        fake_model_file = (std::filesystem::current_path() / "ggml-quijote-test.bin").string();
        std::ofstream ofs_model(fake_model_file);
        ofs_model << "model data";
        ofs_model.close();

        dummy_ctx = reinterpret_cast<whisper_context*>(0xABC123);
        EXPECT_CALL(*mock_engine, init_from_file_with_params(fake_model_file, _))
            .WillOnce(Return(dummy_ctx));

        ISpeechRecognition::speech_recognition_config_t config;
        config.model_name = fake_model_file;
        config.use_gpu = false;
        whisper_recog->initialize(config);
    }

    void TearDown() override {
        if (dummy_ctx) {
            EXPECT_CALL(*mock_engine, free_context(dummy_ctx)).Times(1);
            whisper_recog.reset();
        }
        if (created_fallback_wav && std::filesystem::exists(quijote_wav_path)) {
            std::filesystem::remove(quijote_wav_path);
        }
        if (std::filesystem::exists(fake_model_file)) {
            std::filesystem::remove(fake_model_file);
        }
    }

    std::shared_ptr<StrictMock<MockWhisperEngineWrapper>> mock_engine;
    std::unique_ptr<WhisperSpeechRecognition> whisper_recog;
    whisper_context* dummy_ctx{nullptr};
    std::string quijote_wav_path;
    std::string fake_model_file;
    bool created_fallback_wav{false};
};

TEST_F(WhisperQuijoteIntegrationTest, Process30SecQuijoteAudioBufferSuccess) {
    // Submit 33-second Don Quijote audio clip
    EXPECT_CALL(*mock_engine, run_full(dummy_ctx, _, _))
        .WillOnce(Return(0)); // 0 indicates success

    std::vector<WhisperSegmentData> mock_whisper_segs = {
        {0, 11000, "En un lugar de la Mancha, de cuyo nombre no quiero acordarme...", 0.96f, -0.04f},
        {11000, 22000, "Una olla de algo más vaca que carnero, salpicón las más noches...", 0.94f, -0.06f},
        {22000, 33000, "El resto della concluían sayo de velarte, calzas de velludo...", 0.95f, -0.05f}
    };

    EXPECT_CALL(*mock_engine, extract_segments(dummy_ctx))
        .WillOnce(Return(mock_whisper_segs));

    rst_code_e sub_res = whisper_recog->submit_task(quijote_wav_path);
    EXPECT_EQ(sub_res, RST_OK);
    EXPECT_EQ(whisper_recog->get_status(), ISpeechRecognition::speech_recognition_status_e::COMPLETED);

    std::vector<TranscriptSegment> segments;
    rst_code_e seg_res = whisper_recog->get_segments(segments);
    EXPECT_EQ(seg_res, RST_OK);
    EXPECT_EQ(segments.size(), 3u);
    EXPECT_TRUE(segments[0].text.find("Mancha") != std::string::npos);
    EXPECT_TRUE(segments[1].text.find("carnero") != std::string::npos);
    EXPECT_TRUE(segments[2].text.find("velludo") != std::string::npos);
}
