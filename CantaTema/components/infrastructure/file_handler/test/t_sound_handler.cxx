#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <cstdint>

#include "file_handler/sound_handler.hpp"

namespace fs = std::filesystem;

class SoundHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a unique temporary directory for this test run
        test_dir = fs::temp_directory_path() / "canta_tema_sound_handler_tests";
        if (fs::exists(test_dir)) {
            fs::remove_all(test_dir);
        }
        fs::create_directories(test_dir);
    }

    void TearDown() override {
        if (fs::exists(test_dir)) {
            fs::remove_all(test_dir);
        }
    }

    // Helper to create a valid 1-second WAV file (PCM, 16-bit, Mono, 44.1kHz)
    // This ensures we have a valid audio file for TagLib to parse without relying on external assets.
    void create_one_second_wav(const fs::path& path) {
        std::ofstream f(path, std::ios::binary);
        
        // RIFF header
        f << "RIFF";
        uint32_t data_size = 44100 * 2; // 1 second * 44100 samples/sec * 2 bytes/sample
        uint32_t chunk_size = 36 + data_size;
        f.write(reinterpret_cast<const char*>(&chunk_size), 4);
        f << "WAVE";
        
        // fmt subchunk
        f << "fmt ";
        uint32_t subchunk1_size = 16;
        f.write(reinterpret_cast<const char*>(&subchunk1_size), 4);
        uint16_t audio_format = 1; // PCM
        f.write(reinterpret_cast<const char*>(&audio_format), 2);
        uint16_t num_channels = 1;
        f.write(reinterpret_cast<const char*>(&num_channels), 2);
        uint32_t sample_rate = 44100;
        f.write(reinterpret_cast<const char*>(&sample_rate), 4);
        uint32_t byte_rate = 44100 * 2;
        f.write(reinterpret_cast<const char*>(&byte_rate), 4);
        uint16_t block_align = 2;
        f.write(reinterpret_cast<const char*>(&block_align), 2);
        uint16_t bits_per_sample = 16;
        f.write(reinterpret_cast<const char*>(&bits_per_sample), 2);
        
        // data subchunk
        f << "data";
        f.write(reinterpret_cast<const char*>(&data_size), 4);
        
        // Write 1 second of silence
        std::vector<char> silence(data_size, 0);
        f.write(silence.data(), data_size);
    }

    fs::path test_dir;
};

TEST_F(SoundHandlerTest, GetRecordedSecondsReturnsCorrectDuration) {
    fs::path wav_path = test_dir / "test_audio.wav";
    create_one_second_wav(wav_path);

    SoundFileHandler handler(wav_path.string());
    
    // We expect 1 second. TagLib typically returns duration in seconds.
    EXPECT_EQ(handler.get_recorded_seconds(), 1);
}

TEST_F(SoundHandlerTest, GetRecordedSecondsReturnsZeroForEmptyFile) {
    fs::path empty_path = test_dir / "empty.wav";
    {
        std::ofstream ofs(empty_path);
    }

    SoundFileHandler handler(empty_path.string());
    
    // Should handle invalid/empty file gracefully by returning 0
    EXPECT_EQ(handler.get_recorded_seconds(), 0);
}

TEST_F(SoundHandlerTest, GetRecordedSecondsReturnsZeroForTextFile) {
    fs::path text_path = test_dir / "not_audio.txt";
    {
        std::ofstream ofs(text_path);
        ofs << "This is just text, not audio.";
    }

    SoundFileHandler handler(text_path.string());
    
    EXPECT_EQ(handler.get_recorded_seconds(), 0);
}

TEST_F(SoundHandlerTest, GetRecordedSecondsReturnsDurationForOpus) {
    fs::path source_file = __FILE__;
    // Navigate up: test -> file_handler -> infrastructure -> components -> CantaTema
    fs::path project_root = source_file.parent_path().parent_path().parent_path().parent_path().parent_path();
    fs::path opus_path = project_root / "example_data" / "subject_es_1_p_1.opus";

    if (fs::exists(opus_path)) {
        SoundFileHandler handler(opus_path.string());
        EXPECT_EQ(handler.get_recorded_seconds(), 377);
    } else {
        GTEST_SKIP() << "Test file subject_es_1_p_1.opus not found at " << opus_path;
    }
}

TEST_F(SoundHandlerTest, GetRecordedSecondsReturnsZeroForInvalidOpus) {
    fs::path invalid_opus = test_dir / "invalid.opus";
    {
        std::ofstream ofs(invalid_opus);
        ofs << "garbage content";
    }

    SoundFileHandler handler(invalid_opus.string());
    
    EXPECT_EQ(handler.get_recorded_seconds(), 0);
}

TEST_F(SoundHandlerTest, GetRecordedSecondsHandlesMissingFile) {
    fs::path missing_path = test_dir / "non_existent.wav";

    SoundFileHandler handler(missing_path.string());
    
    EXPECT_EQ(handler.get_recorded_seconds(), 0);
}