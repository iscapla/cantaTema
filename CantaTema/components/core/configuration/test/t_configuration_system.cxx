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
    unsigned int orig_pdf_page_limit = config.get_max_pdf_page_count();
    unsigned int orig_duration_limit = config.get_max_recording_duration_minutes();
    int orig_gpu_layers = config.get_embeddings_gpu_offload_layers();
    float orig_sim_thresh = config.get_coverage_similarity_threshold();
    float orig_bold_w = config.get_importance_weight_bold();
    float orig_italic_w = config.get_importance_weight_italic();
    float orig_underline_w = config.get_importance_weight_underline();
    float orig_bg_color_w = config.get_importance_weight_bg_color();

    // 2. Set garbage/non-parseable values
    (config.*set_fn)("USER_LIMITS", "max_text_file_size_mb", "not_a_number");
    (config.*set_fn)("USER_LIMITS", "max_sound_file_size_mb", "not_a_number");
    (config.*set_fn)("USER_LIMITS", "usage_limit_mb", "not_a_number");
    (config.*set_fn)("USER_LIMITS", "max_pdf_page_count", "not_a_number");
    (config.*set_fn)("USER_LIMITS", "max_recording_duration_minutes", "not_a_number");
    (config.*set_fn)("EMBEDDINGS", "gpu_offload_layers", "not_a_number");
    (config.*set_fn)("COVERAGE", "similarity_threshold", "not_a_number");
    (config.*set_fn)("COVERAGE", "importance_weight_bold", "not_a_number");
    (config.*set_fn)("COVERAGE", "importance_weight_italic", "not_a_number");
    (config.*set_fn)("COVERAGE", "importance_weight_underline", "not_a_number");
    (config.*set_fn)("COVERAGE", "importance_weight_bg_color", "not_a_number");

    // 3. Verify fallbacks are returned
    EXPECT_EQ(config.get_user_default_max_text_file_size_in_mb(), 25u);
    EXPECT_EQ(config.get_user_default_max_sound_file_size_in_mb(), 25u);
    EXPECT_EQ(config.get_user_usage_limit_in_mb(), 512u);
    EXPECT_EQ(config.get_max_pdf_page_count(), 100u);
    EXPECT_EQ(config.get_max_recording_duration_minutes(), 30u);
    EXPECT_EQ(config.get_embeddings_gpu_offload_layers(), 99);
    EXPECT_FLOAT_EQ(config.get_coverage_similarity_threshold(), 0.75f);
    EXPECT_FLOAT_EQ(config.get_importance_weight_bold(), 1.5f);
    EXPECT_FLOAT_EQ(config.get_importance_weight_italic(), 1.2f);
    EXPECT_FLOAT_EQ(config.get_importance_weight_underline(), 1.3f);
    EXPECT_FLOAT_EQ(config.get_importance_weight_bg_color(), 1.4f);

    // 4. Restore original values
    (config.*set_fn)("USER_LIMITS", "max_text_file_size_mb", std::to_string(orig_text_limit));
    (config.*set_fn)("USER_LIMITS", "max_sound_file_size_mb", std::to_string(orig_sound_limit));
    (config.*set_fn)("USER_LIMITS", "usage_limit_mb", std::to_string(orig_usage_limit));
    (config.*set_fn)("USER_LIMITS", "max_pdf_page_count", std::to_string(orig_pdf_page_limit));
    (config.*set_fn)("USER_LIMITS", "max_recording_duration_minutes", std::to_string(orig_duration_limit));
    (config.*set_fn)("EMBEDDINGS", "gpu_offload_layers", std::to_string(orig_gpu_layers));
    (config.*set_fn)("COVERAGE", "similarity_threshold", std::to_string(orig_sim_thresh));
    (config.*set_fn)("COVERAGE", "importance_weight_bold", std::to_string(orig_bold_w));
    (config.*set_fn)("COVERAGE", "importance_weight_italic", std::to_string(orig_italic_w));
    (config.*set_fn)("COVERAGE", "importance_weight_underline", std::to_string(orig_underline_w));
    (config.*set_fn)("COVERAGE", "importance_weight_bg_color", std::to_string(orig_bg_color_w));
}

TEST_F(ConfigurationSystemTest, NewGettersValues) {
    ConfigurationSystem& config = ConfigurationSystem::getInstance();
    EXPECT_STRNE(config.get_whisper_default_model().c_str(), "");
    EXPECT_TRUE(config.get_whisper_use_gpu());
    EXPECT_STRNE(config.get_embeddings_default_model().c_str(), "");
    EXPECT_GE(config.get_embeddings_gpu_offload_layers(), 0);
    EXPECT_GT(config.get_coverage_similarity_threshold(), 0.0f);
    EXPECT_GT(config.get_importance_weight_bold(), 0.0f);
    EXPECT_GT(config.get_importance_weight_italic(), 0.0f);
    EXPECT_GT(config.get_importance_weight_underline(), 0.0f);
    EXPECT_GT(config.get_importance_weight_bg_color(), 0.0f);
    EXPECT_GT(config.get_max_pdf_page_count(), 0u);
    EXPECT_GT(config.get_max_recording_duration_minutes(), 0u);
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