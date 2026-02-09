#include <gtest/gtest.h>
#include <sound_system/sound_system.hpp>
#include <filesystem>
#include <fstream>
#include <thread>
#include <chrono>
#include <cstring>

namespace {

class SoundSystemTest : public ::testing::Test {
protected:
    const std::string kTestFile = "test_audio.opus";
    const std::string kEncryptedFile = "test_audio_enc.opus";
    
    void SetUp() override {
        cleanup();
    }

    void TearDown() override {
        cleanup();
    }

    void cleanup() {
        std::error_code ec;
        if (std::filesystem::exists(kTestFile)) std::filesystem::remove(kTestFile, ec);
        if (std::filesystem::exists(kEncryptedFile)) std::filesystem::remove(kEncryptedFile, ec);
    }

    bool has_capture_devices() {
        SoundSystem ss(ISoundSystem::SoundSystemConfig{});
        return !ss.getCaptureDevices().empty();
    }
};

TEST_F(SoundSystemTest, ConstructAndDestruct) {
    EXPECT_NO_THROW({
        SoundSystem ss(ISoundSystem::SoundSystemConfig{});
    });
}

TEST_F(SoundSystemTest, GetCaptureDevices) {
    SoundSystem ss(ISoundSystem::SoundSystemConfig{});
    std::vector<SoundSystem::SoundSystemDeviceInfo> devices;
    EXPECT_NO_THROW({
        devices = ss.getCaptureDevices();
    });
    
    for (const auto& dev : devices) {
        EXPECT_FALSE(dev.name.empty());
    }
}

TEST_F(SoundSystemTest, PlaybackStateManagement) {
    SoundSystem ss(ISoundSystem::SoundSystemConfig{});
    
    EXPECT_FALSE(ss.isPlaying());
    EXPECT_EQ(ss.get_playing_timestamp(), 0);

    EXPECT_NO_THROW(ss.stopPlaying());
}

TEST_F(SoundSystemTest, PlayInvalidFile) {
    SoundSystem ss(ISoundSystem::SoundSystemConfig{});
    bool result = ss.play("non_existent_random_file_12345.opus");
    EXPECT_FALSE(result);
    EXPECT_FALSE(ss.isPlaying());
}

TEST_F(SoundSystemTest, RecordingStateManagement) {
    SoundSystem ss(ISoundSystem::SoundSystemConfig{});
    
    EXPECT_FALSE(ss.isRecording());
    EXPECT_EQ(ss.get_recording_timestamp(), 0);
    
    EXPECT_NO_THROW(ss.stopRecording());
}

TEST_F(SoundSystemTest, RecordAndPlayPlain) {
    if (!has_capture_devices()) {
        SUCCEED() << "No capture devices available, skipping test.";
        return;
    }

    // 1. Record
    {
        SoundSystem ss(ISoundSystem::SoundSystemConfig{});
        EXPECT_TRUE(ss.startRecording(kTestFile));
        EXPECT_TRUE(ss.isRecording());
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        EXPECT_GE(ss.get_recording_timestamp(), 0);
        ss.stopRecording();
        EXPECT_FALSE(ss.isRecording());
    }
    
    EXPECT_TRUE(std::filesystem::exists(kTestFile));

    // 2. Play
    {
        SoundSystem ss(ISoundSystem::SoundSystemConfig{});
        EXPECT_TRUE(ss.play(kTestFile));
        if (ss.isPlaying()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            EXPECT_GE(ss.get_playing_timestamp(), 0);
            ss.stopPlaying();
            EXPECT_FALSE(ss.isPlaying());
        }
    }
}

TEST_F(SoundSystemTest, RecordAndPlayEncrypted) {
    if (!has_capture_devices()) {
        SUCCEED() << "No capture devices available, skipping test.";
        return;
    }

    std::string key = "SecretKey";
    ISoundSystem::SoundSystemConfig config;
    config.encryptionKey = key;

    // 1. Record Encrypted
    {
        SoundSystem ss(config);
        EXPECT_TRUE(ss.startRecording(kEncryptedFile));
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        ss.stopRecording();
    }
    EXPECT_TRUE(std::filesystem::exists(kEncryptedFile));

    // 2. Verify File Header is Encrypted (Not "OggS")
    {
        std::ifstream f(kEncryptedFile, std::ios::binary);
        ASSERT_TRUE(f.is_open());
        char header[4];
        f.read(header, 4);
        EXPECT_NE(std::memcmp(header, "OggS", 4), 0);
    }

    // 3. Play with Correct Key
    {
        SoundSystem ss(config);
        EXPECT_TRUE(ss.play(kEncryptedFile));
        if (ss.isPlaying()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            ss.stopPlaying();
        }
    }
}

} // namespace