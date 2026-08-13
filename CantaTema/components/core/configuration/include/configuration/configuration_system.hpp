
#ifndef CONFIGURATION_SYSTEM_HPP
#define CONFIGURATION_SYSTEM_HPP

#include <string>

#include "configuration/i_configuration_base.hpp"

class ConfigurationSystem : public IConfigurationBase {
public:
    static ConfigurationSystem& getInstance();

    static const unsigned int MAX_EXTENSIONS_LENGTH = 20;
    static const unsigned int MAX_EXTENSIONS_COUNT = 5;

    ConfigurationSystem(const ConfigurationSystem&) = delete;
    ConfigurationSystem& operator=(const ConfigurationSystem&) = delete;

    /**
     * @brief Retrieves the allowed text file extensions from the configuration.
     * 
     * @param patterns Array of character pointers to be populated with extension strings.
     * @param max_patterns The maximum number of patterns the array can hold.
     * @param max_extension_size The maximum size of each extension string.
     * @return int The number of extensions actually retrieved and stored in patterns.
     */
    int get_text_files_extensions_allowed(char patterns[MAX_EXTENSIONS_COUNT][MAX_EXTENSIONS_LENGTH]) const;

    /**
     * @brief Retrieves the default maximum text file size allowed for a user in Megabytes.
     * 
     * @return unsigned int The maximum file size in MB.
     */
    unsigned int get_user_default_max_text_file_size_in_mb(void) const;

    /**
     * @brief Retrieves the allowed sound file extensions from the configuration.
     * 
     * @param patterns Array of character pointers to be populated with extension strings.
     * @param max_patterns The maximum number of patterns the array can hold.
     * @param max_extension_size The maximum size of each extension string.
     * @return int The number of extensions actually retrieved and stored in patterns.
     */
    int get_sound_files_extensions_allowed(char patterns[MAX_EXTENSIONS_COUNT][MAX_EXTENSIONS_LENGTH]) const;

    /**
     * @brief Retrieves the default maximum sound file size allowed for a user in Megabytes.
     * 
     * @return unsigned int The maximum file size in MB.
     */
    unsigned int get_user_default_max_sound_file_size_in_mb(void) const;

    /**
     * @brief Retrieves the total usage limit allowed for a user in Megabytes.
     * 
     * @return unsigned int The usage limit in MB.
     */
    unsigned int get_user_usage_limit_in_mb(void) const;

    /**
     * @brief Retrieves the default whisper model name (e.g. "AUTO").
     */
    std::string get_whisper_default_model() const;

    /**
     * @brief Retrieves whether GPU acceleration is enabled for Whisper (default false).
     */
    bool get_whisper_use_gpu() const;

    /**
     * @brief Retrieves the default embedding model name (e.g. "AUTO").
     */
    std::string get_embeddings_default_model() const;

    /**
     * @brief Retrieves the number of layers to offload to the GPU for llama embeddings.
     */
    int get_embeddings_gpu_offload_layers() const;

    /**
     * @brief Retrieves whether asymmetric prompt prefixes (passage:/query:) are enabled for embedding generation.
     */
    bool get_embeddings_use_role_prefixes() const;

    /**
     * @brief Retrieves the prefix prepended to reference document passage texts.
     */
    std::string get_embeddings_passage_prefix() const;

    /**
     * @brief Retrieves the prefix prepended to voice transcript query texts.
     */
    std::string get_embeddings_query_prefix() const;

    /**
     * @brief Retrieves the threshold score for cosine similarity.
     */
    float get_coverage_similarity_threshold() const;

    /**
     * @brief Retrieves the minimum word count required for an extracted reference chunk.
     */
    unsigned int get_coverage_min_chunk_word_count() const;

    /**
     * @brief Retrieves the similarity score boost for matching dates/numbers.
     */
    float get_coverage_numeric_boost() const;

    /**
     * @brief Retrieves the penalty applied when dates/numbers conflict.
     */
    float get_coverage_numeric_mismatch_penalty() const;

    /**
     * @brief Retrieves the position distance penalty weight for macroscopic prose ordering.
     */
    float get_coverage_temporal_penalty_weight() const;

    /**
     * @brief Retrieves the maximum word count to classify a chunk as a short chunk / heading requiring keyword validation.
     */
    unsigned int get_coverage_short_chunk_word_threshold() const;

    /**
     * @brief Retrieves the scaling factor multiplier applied when zero keyword overlap occurs for a short chunk.
     */
    float get_coverage_lexical_mismatch_scaling_factor() const;

    /**
     * @brief Retrieves the weight multiplier for bold formatted text.
     */
    float get_importance_weight_bold() const;

    /**
     * @brief Retrieves the weight multiplier for italic formatted text.
     */
    float get_importance_weight_italic() const;

    /**
     * @brief Retrieves the weight multiplier for underlined text.
     */
    float get_importance_weight_underline() const;

    /**
     * @brief Retrieves the weight multiplier for background colored text.
     */
    float get_importance_weight_bg_color() const;

    /**
     * @brief Retrieves the maximum PDF page limit for ingestion.
     */
    unsigned int get_max_pdf_page_count() const;

    /**
     * @brief Retrieves the maximum recording duration in minutes.
     */
    unsigned int get_max_recording_duration_minutes() const;

    /**
     * @brief Retrieves the active language code for comparison (e.g. "es", "en").
     */
    std::string get_comparison_active_language() const;

    /**
     * @brief Retrieves the active domain key for comparison (e.g. "law", "economics", "history", "science", "general").
     */
    std::string get_comparison_active_domain() const;

    /**
     * @brief Retrieves whether 2nd-pass CTC forced alignment is enabled.
     */
    bool get_alignment_enable_ctc_pass() const;

    /**
     * @brief Retrieves the alignment mode (e.g. "AUTO", "whisper_ctc").
     */
    std::string get_alignment_mode() const;

    /**
     * @brief Retrieves the maximum drift tolerance in milliseconds for CTC alignment.
     */
    unsigned int get_alignment_max_drift_tolerance_ms() const;

    /**
     * @brief Retrieves whether phonetic ASR noise compensation is enabled.
     */
    bool get_phonetic_enable_matching() const;

    /**
     * @brief Retrieves the default phonetic matcher algorithm (e.g. "double_metaphone", "soundex").
     */
    std::string get_phonetic_default_matcher() const;

    /**
     * @brief Retrieves the similarity score threshold for classifying phonetic mispronunciations.
     */
    float get_phonetic_similarity_threshold() const;

    /**
     * @brief Sets a configuration value at runtime (primarily for testing).
     */
    rst_code_e set_value(const std::string& section, const std::string& field, const std::string& value);




private:
    ConfigurationSystem();
    ~ConfigurationSystem();

    inline static const std::string config_file_name = "system.ini";

    /**
     * @brief Retrieves the allowed file extensions from the configuration.
     * 
     * @param patterns Array of character pointers to be populated with extension strings.
     * @param max_patterns The maximum number of patterns the array can hold.
     * @param max_extension_size The maximum size of each extension string.
     * @return int The number of extensions actually retrieved and stored in patterns.
     */
    int get_files_extensions_allowed(const std::string section, const std::string key, char patterns[MAX_EXTENSIONS_COUNT][MAX_EXTENSIONS_LENGTH]) const;

};

#endif // CONFIGURATION_SYSTEM_HPP