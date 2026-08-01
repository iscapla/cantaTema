#include <gtest/gtest.h>
#include "primitives/user_configuration.hpp"

TEST(UserConfigurationTest, DefaultValuesInitialization) {
    UserConfiguration config;
    
    EXPECT_EQ(config.whisper.model_name, "AUTO");
    EXPECT_EQ(config.whisper.language, "es");
    EXPECT_FALSE(config.whisper.use_gpu);
    EXPECT_TRUE(config.whisper.enable_timestamps);
    EXPECT_TRUE(config.whisper.enable_confidence);
    EXPECT_EQ(config.whisper.thread_count, 4);

    EXPECT_EQ(config.reference_extraction.max_pdf_page_count, 50u);
    EXPECT_FLOAT_EQ(config.reference_extraction.importance_weight_bold, 1.5f);
    EXPECT_FLOAT_EQ(config.reference_extraction.importance_weight_italic, 1.2f);
    EXPECT_FLOAT_EQ(config.reference_extraction.importance_weight_underline, 1.3f);
    EXPECT_FLOAT_EQ(config.reference_extraction.importance_weight_bg_color, 2.0f);
    EXPECT_EQ(config.reference_extraction.min_chunk_word_count, 3u);

    EXPECT_EQ(config.comparison.embedding_model_name, "AUTO");
    EXPECT_EQ(config.comparison.embedding_gpu_offload_layers, 0);
    EXPECT_FLOAT_EQ(config.comparison.similarity_threshold, 0.65f);
    EXPECT_FLOAT_EQ(config.comparison.speed_weight, 0.3f);
    EXPECT_FLOAT_EQ(config.comparison.clarity_weight, 0.4f);
    EXPECT_FLOAT_EQ(config.comparison.pacing_weight, 0.3f);
}

TEST(UserConfigurationTest, ResetDefaultValues) {
    UserConfiguration config;
    config.whisper.model_name = "custom_model.bin";
    config.whisper.use_gpu = true;
    config.reference_extraction.importance_weight_bold = 3.0f;
    config.comparison.similarity_threshold = 0.85f;

    config.set_default_values();

    EXPECT_EQ(config.whisper.model_name, "AUTO");
    EXPECT_FALSE(config.whisper.use_gpu);
    EXPECT_FLOAT_EQ(config.reference_extraction.importance_weight_bold, 1.5f);
    EXPECT_FLOAT_EQ(config.comparison.similarity_threshold, 0.65f);
}

TEST(UserConfigurationTest, JsonSerializationAndDeserialization) {
    UserConfiguration original;
    original.whisper.model_name = "ggml-medium.bin";
    original.whisper.language = "en";
    original.whisper.use_gpu = true;
    original.reference_extraction.importance_weight_bold = 2.5f;
    original.comparison.similarity_threshold = 0.80f;

    std::string json_output = original.to_json();
    EXPECT_FALSE(json_output.empty());

    UserConfiguration restored;
    bool success = restored.from_json(json_output);
    EXPECT_TRUE(success);

    EXPECT_EQ(restored.whisper.model_name, "ggml-medium.bin");
    EXPECT_EQ(restored.whisper.language, "en");
    EXPECT_TRUE(restored.whisper.use_gpu);
    EXPECT_FLOAT_EQ(restored.reference_extraction.importance_weight_bold, 2.5f);
    EXPECT_FLOAT_EQ(restored.comparison.similarity_threshold, 0.80f);
}
