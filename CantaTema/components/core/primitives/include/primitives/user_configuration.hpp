/**
 * @file user_configuration.hpp
 * @brief Configuration data structures and UserConfiguration class for per-user execution settings.
 */

#ifndef USER_CONFIGURATION_HPP
#define USER_CONFIGURATION_HPP

#include <string>
#include <cstdint>
#include "primitives/definitions.hpp"

/**
 * @brief Configuration parameters for Whisper speech recognition decoding.
 */
struct WhisperConfig {
    std::string model_name{"AUTO"};
    std::string language{"es"};
    bool use_gpu{false};
    bool enable_timestamps{true};
    bool enable_confidence{true};
    int thread_count{4};

    bool operator==(const WhisperConfig& other) const = default;
};

/**
 * @brief Configuration parameters for reference text extraction (PDF / text).
 */
struct ReferenceExtractionConfig {
    unsigned int max_pdf_page_count{50};
    float importance_weight_bold{1.5f};
    float importance_weight_italic{1.2f};
    float importance_weight_underline{1.3f};
    float importance_weight_bg_color{2.0f};
    size_t min_chunk_word_count{3};

    bool operator==(const ReferenceExtractionConfig& other) const = default;
};

/**
 * @brief Configuration parameters for embedding generation, similarity matching, and scoring.
 */
struct ComparisonConfig {
    std::string embedding_model_name{"AUTO"};
    int embedding_gpu_offload_layers{0};
    bool use_role_prefixes{true};
    std::string passage_prefix{"passage: "};
    std::string query_prefix{"query: "};
    float similarity_threshold{0.65f};
    float numeric_boost{0.10f};
    float numeric_mismatch_penalty{0.15f};
    float temporal_penalty_weight{0.05f};
    unsigned int short_chunk_word_threshold{10u};
    float lexical_mismatch_scaling_factor{0.60f};
    float speed_weight{0.3f};
    float clarity_weight{0.4f};
    float pacing_weight{0.3f};

    bool operator==(const ComparisonConfig& other) const = default;
};

/**
 * @class UserConfiguration
 * @brief Unified configuration object encapsulating per-user parameters for Whisper speech recognition,
 * reference text extraction, and semantic comparison analysis.
 */
class UserConfiguration {
public:
    WhisperConfig whisper;
    ReferenceExtractionConfig reference_extraction;
    ComparisonConfig comparison;

    /**
     * @brief Construct a new UserConfiguration and initialize all fields with default values.
     */
    UserConfiguration();

    /**
     * @brief Sets all configuration parameters to default values.
     * 
     * Resets whisper, reference extraction, and comparison fields to baseline system defaults.
     */
    void set_default_values();

    /**
     * @brief Serializes the configuration into a JSON formatted string.
     * @return std::string JSON representation of the configuration.
     */
    std::string to_json() const;

    /**
     * @brief Deserializes configuration values from a JSON string.
     * @param json_str JSON string containing configuration parameters.
     * @return bool True if parsing was successful, false otherwise.
     */
    bool from_json(const std::string& json_str);

    bool operator==(const UserConfiguration& other) const = default;
};

#endif // USER_CONFIGURATION_HPP
