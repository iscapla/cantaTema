#include <gtest/gtest.h>
#include <string>

#include "configuration/configuration_system.hpp"

class ConfigurationSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Code here will be called immediately after the constructor (right
        // before each test).
    }

    void TearDown() override {
        // Code here will be called immediately after each test (right
        // before the destructor).
    }
};

TEST_F(ConfigurationSystemTest, SingletonInstance) {
    ConfigurationSystem& instance1 = ConfigurationSystem::getInstance();
    ConfigurationSystem& instance2 = ConfigurationSystem::getInstance();
    EXPECT_EQ(&instance1, &instance2);
}

TEST_F(ConfigurationSystemTest, TextFilesExtensionsAllowed) {
    ConfigurationSystem& config = ConfigurationSystem::getInstance();
    char patterns[ConfigurationSystem::MAX_EXTENSIONS_COUNT][ConfigurationSystem::MAX_EXTENSIONS_LENGTH];
    
    int count = config.get_text_files_extensions_allowed(patterns);
    
    EXPECT_GE(count, 0);
    EXPECT_LE(count, static_cast<int>(ConfigurationSystem::MAX_EXTENSIONS_COUNT));
    
    for (int i = 0; i < count; ++i) {
        EXPECT_STRNE(patterns[i], "");
        // Ensure null termination and length safety
        std::string ext(patterns[i]);
        EXPECT_LT(ext.length(), static_cast<size_t>(ConfigurationSystem::MAX_EXTENSIONS_LENGTH));
    }
}

TEST_F(ConfigurationSystemTest, UserDefaultMaxTextFileSize) {
    ConfigurationSystem& config = ConfigurationSystem::getInstance();
    unsigned int size = config.get_user_default_max_text_file_size_in_mb();
    EXPECT_GT(size, 0u);
}

TEST_F(ConfigurationSystemTest, SoundFilesExtensionsAllowed) {
    ConfigurationSystem& config = ConfigurationSystem::getInstance();
    char patterns[ConfigurationSystem::MAX_EXTENSIONS_COUNT][ConfigurationSystem::MAX_EXTENSIONS_LENGTH];
    
    int count = config.get_sound_files_extensions_allowed(patterns);
    
    EXPECT_GE(count, 0);
    EXPECT_LE(count, static_cast<int>(ConfigurationSystem::MAX_EXTENSIONS_COUNT));
    
    for (int i = 0; i < count; ++i) {
        EXPECT_STRNE(patterns[i], "");
        std::string ext(patterns[i]);
        EXPECT_LT(ext.length(), static_cast<size_t>(ConfigurationSystem::MAX_EXTENSIONS_LENGTH));
    }
}

TEST_F(ConfigurationSystemTest, UserDefaultMaxSoundFileSize) {
    ConfigurationSystem& config = ConfigurationSystem::getInstance();
    unsigned int size = config.get_user_default_max_sound_file_size_in_mb();
    EXPECT_GT(size, 0u);
}

TEST_F(ConfigurationSystemTest, UserUsageLimit) {
    ConfigurationSystem& config = ConfigurationSystem::getInstance();
    unsigned int limit = config.get_user_usage_limit_in_mb();
    EXPECT_GT(limit, 0u);
}