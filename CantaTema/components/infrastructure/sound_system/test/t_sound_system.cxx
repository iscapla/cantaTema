#include <gtest/gtest.h>
#include <sound_system/sound_system.hpp>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>

namespace {

class SoundSystemTest : public ::testing::Test {
protected:
    const std::string kTestWavFile = "test_audio_sample.wav";
    const std::string kTestRecFile = "test_rec_output.wav";

    void SetUp() override {
        // Create a minimal valid WAV file for playback testing
        create_dummy_wav(kTestWavFile);
    }

    void TearDown() override {
        // Cleanup files
        std::error_code ec;
        if (std::filesystem::exists(kTestWavFile)) {
            std::filesystem::remove(kTestWavFile, ec);
        }
        if (std::filesystem::exists(kTestRecFile)) {
            std::filesystem::remove(kTestRecFile, ec);
        }
    }

    // Helper to create a dummy WAV file (PCM, 16-bit, Mono, 44100Hz)
    void create_dummy_wav(const std::string& filename) {
        std::ofstream f(filename, std::ios::binary);
        if (!f.is_open()) return;

        // Total file size - 8. We'll write 0 bytes of data, so 36 bytes for header.
        uint32_t data_size = 0;
        uint32_t chunk_size = 36 + data_size;
        
        f.write("RIFF", 4);
        f.write(reinterpret_cast<const char*>(&chunk_size), 4);
        f.write("WAVE", 4);
        
        f.write("fmt ", 4);
        uint32_t subchunk1_size = 16; // PCM
        f.write(reinterpret_cast<const char*>(&subchunk1_size), 4);
        
        uint16_t audio_format = 1; // PCM
        f.write(reinterpret_cast<const char*>(&audio_format), 2);
        
        uint16_t num_channels = 1;
        f.write(reinterpret_cast<const char*>(&num_channels), 2);
        
        uint32_t sample_rate = 44100;
        f.write(reinterpret_cast<const char*>(&sample_rate), 4);
        
        uint32_t byte_rate = sample_rate * num_channels * 2; // 16 bits = 2 bytes
        f.write(reinterpret_cast<const char*>(&byte_rate), 4);
        
        uint16_t block_align = num_channels * 2;
        f.write(reinterpret_cast<const char*>(&block_align), 2);
        
        uint16_t bits_per_sample = 16;
        f.write(reinterpret_cast<const char*>(&bits_per_sample), 2);
        
        f.write("data", 4);
        f.write(reinterpret_cast<const char*>(&data_size), 4);
    }
};

TEST_F(SoundSystemTest, ConstructAndDestruct) {
    EXPECT_NO_THROW({
        SoundSystem ss;
    });
}

TEST_F(SoundSystemTest, GetCaptureDevices) {
    SoundSystem ss;
    std::vector<SoundSystem::DeviceInfo> devices;
    EXPECT_NO_THROW({
        devices = ss.getCaptureDevices();
    });
    
    for (const auto& dev : devices) {
        EXPECT_FALSE(dev.name.empty());
    }
}

TEST_F(SoundSystemTest, PlaybackStateManagement) {
    SoundSystem ss;
    
    EXPECT_FALSE(ss.isPlaying());
    EXPECT_EQ(ss.get_playing_timestamp(), 0);

    EXPECT_NO_THROW(ss.stopPlaying());
}

TEST_F(SoundSystemTest, PlayInvalidFile) {
    SoundSystem ss;
    bool result = ss.play("non_existent_random_file_12345.wav");
    EXPECT_FALSE(result);
    EXPECT_FALSE(ss.isPlaying());
}

TEST_F(SoundSystemTest, PlayValidFile) {
    SoundSystem ss;
    
    // Attempt to play the dummy file
    bool started = ss.play(kTestWavFile);
    
    // Depending on the environment (e.g. CI without audio device), this might return false.
    // We check consistency based on the result.
    if (started) {
        EXPECT_TRUE(ss.isPlaying());
        
        // Let it run briefly
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        // Timestamp check
        EXPECT_GE(ss.get_playing_timestamp(), 0);
        
        ss.stopPlaying();
        EXPECT_FALSE(ss.isPlaying());
    } else {
        EXPECT_FALSE(ss.isPlaying());
    }
}

TEST_F(SoundSystemTest, RecordingStateManagement) {
    SoundSystem ss;
    
    EXPECT_FALSE(ss.isRecording());
    EXPECT_EQ(ss.get_recording_timestamp(), 0);
    
    EXPECT_NO_THROW(ss.stopRecording());
}

TEST_F(SoundSystemTest, RecordingFlow) {
    SoundSystem ss;
    
    auto devices = ss.getCaptureDevices();
    if (devices.empty()) {
        SUCCEED() << "No capture devices available, skipping recording test.";
        return;
    }

    // Try to record
    bool started = ss.startRecording(kTestRecFile);
    if (started) {
        EXPECT_TRUE(ss.isRecording());
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        EXPECT_GE(ss.get_recording_timestamp(), 0);
        ss.stopRecording();
        EXPECT_FALSE(ss.isRecording());
        EXPECT_TRUE(std::filesystem::exists(kTestRecFile));
    } else {
        EXPECT_FALSE(ss.isRecording());
    }
}

} // namespace