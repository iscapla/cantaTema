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


// Conforming C++ access control bypass for testing protected IConfigurationBase::set
template<typename Tag, typename Tag::type M>
struct Rob {
  friend typename Tag::type get(Tag) {
    return M;
  }
};

struct IConfigurationBase_set {
  typedef rst_code_e (IConfigurationBase::*type)(const std::string&, const std::string&, const std::string&);
  friend type get(IConfigurationBase_set);
};

template struct Rob<IConfigurationBase_set, &IConfigurationBase::set>;

TEST_F(ConfigurationSystemTest, FallbackOnGarbageValues) {
    ConfigurationSystem& config = ConfigurationSystem::getInstance();
    auto set_fn = get(IConfigurationBase_set());

    // 1. Save original values
    unsigned int orig_text_limit = config.get_user_default_max_text_file_size_in_mb();
    unsigned int orig_sound_limit = config.get_user_default_max_sound_file_size_in_mb();
    unsigned int orig_usage_limit = config.get_user_usage_limit_in_mb();

    // 2. Set garbage/non-parseable values
    (config.*set_fn)("USER_LIMITS", "max_text_file_size_mb", "not_a_number");
    (config.*set_fn)("USER_LIMITS", "max_sound_file_size_mb", "not_a_number");
    (config.*set_fn)("USER_LIMITS", "usage_limit_mb", "not_a_number");

    // 3. Verify fallbacks are returned
    EXPECT_EQ(config.get_user_default_max_text_file_size_in_mb(), 10u);
    EXPECT_EQ(config.get_user_default_max_sound_file_size_in_mb(), 50u);
    EXPECT_EQ(config.get_user_usage_limit_in_mb(), 128u);

    // 4. Restore original values
    (config.*set_fn)("USER_LIMITS", "max_text_file_size_mb", std::to_string(orig_text_limit));
    (config.*set_fn)("USER_LIMITS", "max_sound_file_size_mb", std::to_string(orig_sound_limit));
    (config.*set_fn)("USER_LIMITS", "usage_limit_mb", std::to_string(orig_usage_limit));
}

class TestConfiguration : public IConfigurationBase {
public:
    using IConfigurationBase::parse;
    using IConfigurationBase::update_values_to_file;
    using IConfigurationBase::set_default_if_not_present;
    using IConfigurationBase::set_file_path;
    using IConfigurationBase::get;
    using IConfigurationBase::set;
};

TEST_F(ConfigurationSystemTest, BaseConfigurationCoverage) {
    TestConfiguration test_config;
    test_config.set_file_path("");
    EXPECT_EQ(test_config.update_values_to_file(), UNKNOWN);

    std::filesystem::path dummy_ini = "dummy_config.ini";
    test_config.set_file_path(dummy_ini);
    EXPECT_EQ(test_config.parse(), RST_OK);

    EXPECT_EQ(test_config.set_default_if_not_present("SEC", "key", "val"), RST_OK);
    EXPECT_EQ(test_config.get("SEC", "key"), "val");
    EXPECT_EQ(test_config.set("SEC", "key", "newval"), RST_OK);
    EXPECT_EQ(test_config.get("SEC", "key"), "newval");

    std::filesystem::remove(dummy_ini);
}