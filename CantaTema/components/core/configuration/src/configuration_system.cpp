
#include <vector>
#include <sstream>
#include <mutex>
#include <cstring>

#include "primitives/tool_paths.hpp"
#include "primitives/utils_logger.hpp"
#include "configuration/configuration_system.hpp"

ConfigurationSystem& ConfigurationSystem::getInstance() {
    static ConfigurationSystem instance;
    return instance;
}

ConfigurationSystem::ConfigurationSystem() : IConfigurationBase() {
    std::filesystem::path db_path = ToolPath::get_path_for_system_config() / config_file_name;
    set_file_path(db_path);
    parse();

    set_default_if_not_present("TEXT_FILES", "extensions_allowed", "*.txt\n*.pdf");
    set_default_if_not_present("SOUND_FILES", "extensions_allowed", "*.opus");
    set_default_if_not_present("USER_LIMITS", "max_text_file_size_mb", "25");
    set_default_if_not_present("USER_LIMITS", "max_sound_file_size_mb", "25");
    set_default_if_not_present("USER_LIMITS", "usage_limit_mb", "512");
    set_default_if_not_present("USER_LIMITS", "max_pdf_page_count", "100");
    set_default_if_not_present("USER_LIMITS", "max_recording_duration_minutes", "30");

    set_default_if_not_present("WHISPER", "default_model", "AUTO");
    set_default_if_not_present("WHISPER", "use_gpu", "true");

    set_default_if_not_present("EMBEDDINGS", "default_model", "AUTO");
    set_default_if_not_present("EMBEDDINGS", "gpu_offload_layers", "99");
    set_default_if_not_present("EMBEDDINGS", "use_role_prefixes", "true");
    set_default_if_not_present("EMBEDDINGS", "passage_prefix", "passage: ");
    set_default_if_not_present("EMBEDDINGS", "query_prefix", "query: ");

    set_default_if_not_present("COVERAGE", "similarity_threshold", "0.75");
    set_default_if_not_present("COVERAGE", "min_chunk_word_count", "4");
    set_default_if_not_present("COVERAGE", "numeric_boost", "0.10");
    set_default_if_not_present("COVERAGE", "numeric_mismatch_penalty", "0.15");
    set_default_if_not_present("COVERAGE", "temporal_penalty_weight", "0.05");
    set_default_if_not_present("COVERAGE", "short_chunk_word_threshold", "10");
    set_default_if_not_present("COVERAGE", "lexical_mismatch_scaling_factor", "0.60");
    set_default_if_not_present("COVERAGE", "importance_weight_bold", "1.5");
    set_default_if_not_present("COVERAGE", "importance_weight_italic", "1.2");
    set_default_if_not_present("COVERAGE", "importance_weight_underline", "1.3");
    set_default_if_not_present("COVERAGE", "importance_weight_bg_color", "1.4");

    update_values_to_file();
}

ConfigurationSystem::~ConfigurationSystem() {
}

/**
 * @brief Retrieves the allowed file extensions from the configuration.
 * 
 * @param patterns Array of character pointers to be populated with extension strings.
 * @return int The number of extensions actually retrieved and stored in patterns.
 */
int ConfigurationSystem::get_files_extensions_allowed(
    const std::string section,
    const std::string key,
    char patterns[MAX_EXTENSIONS_COUNT][MAX_EXTENSIONS_LENGTH]
) const {

    std::string default_val = (section == "TEXT_FILES") ? "*.txt\n*.pdf" : "*.opus";
    std::string value = const_cast<ConfigurationSystem*>(this)->get_or_default(section, key, default_val);

    if(patterns == nullptr){
        logger->error("Destination variable cannot be null.");
        return 0;
    }

    static std::vector<std::string> cache;
    static std::mutex cache_mtx;
    std::lock_guard<std::mutex> lock(cache_mtx);

    cache.clear();
    std::stringstream ss(value);
    std::string segment;
    while (std::getline(ss, segment, '\n')) {
        if (!segment.empty() && segment.length() < MAX_EXTENSIONS_LENGTH) {
            cache.push_back(segment);
        }
    }

    int count = 0;
    for (size_t i = 0; i < cache.size() && i < MAX_EXTENSIONS_COUNT; ++i) {
        strncpy(patterns[i], cache[i].c_str(), MAX_EXTENSIONS_LENGTH - 1);
        patterns[i][MAX_EXTENSIONS_LENGTH - 1] = '\0';
        count++;
    }
    return count;
}

/**
 * @brief Retrieves the allowed text file extensions from the configuration.
 * 
 * @param patterns Array of character pointers to be populated with extension strings.
 * @return int The number of extensions actually retrieved and stored in patterns.
 */
int ConfigurationSystem::get_text_files_extensions_allowed(char patterns[MAX_EXTENSIONS_COUNT][MAX_EXTENSIONS_LENGTH]) const {
    return get_files_extensions_allowed("TEXT_FILES", "extensions_allowed", patterns);
}

unsigned int ConfigurationSystem::get_user_default_max_text_file_size_in_mb(void) const {
    std::string value = const_cast<ConfigurationSystem*>(this)->get_or_default("USER_LIMITS", "max_text_file_size_mb", "25");
    try {
        return std::stoul(value);
    } catch (...) {
        return 25; // Default fallback
    }
}

/**
 * @brief Retrieves the allowed sound file extensions from the configuration.
 * 
 * @param patterns Array of character pointers to be populated with extension strings.
 * @return int The number of extensions actually retrieved and stored in patterns.
 */
int ConfigurationSystem::get_sound_files_extensions_allowed(char patterns[MAX_EXTENSIONS_COUNT][MAX_EXTENSIONS_LENGTH]) const {
    return get_files_extensions_allowed("SOUND_FILES", "extensions_allowed", patterns);
}

unsigned int ConfigurationSystem::get_user_default_max_sound_file_size_in_mb(void) const {
    std::string value = const_cast<ConfigurationSystem*>(this)->get_or_default("USER_LIMITS", "max_sound_file_size_mb", "25");
    try {
        return std::stoul(value);
    } catch (...) {
        return 25; // Default fallback
    }
}

unsigned int ConfigurationSystem::get_user_usage_limit_in_mb(void) const {
    std::string value = const_cast<ConfigurationSystem*>(this)->get_or_default("USER_LIMITS", "usage_limit_mb", "512");
    try {
        return std::stoul(value);
    } catch (...) {
        return 512; // Default fallback
    }
}

std::string ConfigurationSystem::get_whisper_default_model() const {
    return const_cast<ConfigurationSystem*>(this)->get_or_default("WHISPER", "default_model", "AUTO");
}

bool ConfigurationSystem::get_whisper_use_gpu() const {
    std::string value = const_cast<ConfigurationSystem*>(this)->get_or_default("WHISPER", "use_gpu", "true");
    if (value == "true" || value == "1" || value == "TRUE") {
        return true;
    }
    return false;
}

std::string ConfigurationSystem::get_embeddings_default_model() const {
    return const_cast<ConfigurationSystem*>(this)->get_or_default("EMBEDDINGS", "default_model", "AUTO");
}

int ConfigurationSystem::get_embeddings_gpu_offload_layers() const {
    std::string value = const_cast<ConfigurationSystem*>(this)->get_or_default("EMBEDDINGS", "gpu_offload_layers", "99");
    try {
        return std::stoi(value);
    } catch (...) {
        return 99; // Default fallback
    }
}

bool ConfigurationSystem::get_embeddings_use_role_prefixes() const {
    std::string value = const_cast<ConfigurationSystem*>(this)->get_or_default("EMBEDDINGS", "use_role_prefixes", "true");
    return (value == "true" || value == "1" || value == "TRUE");
}

std::string ConfigurationSystem::get_embeddings_passage_prefix() const {
    return const_cast<ConfigurationSystem*>(this)->get_or_default("EMBEDDINGS", "passage_prefix", "passage: ");
}

std::string ConfigurationSystem::get_embeddings_query_prefix() const {
    return const_cast<ConfigurationSystem*>(this)->get_or_default("EMBEDDINGS", "query_prefix", "query: ");
}

float ConfigurationSystem::get_coverage_similarity_threshold() const {
    std::string value = const_cast<ConfigurationSystem*>(this)->get_or_default("COVERAGE", "similarity_threshold", "0.75");
    try {
        return std::stof(value);
    } catch (...) {
        return 0.75f; // Default fallback
    }
}

unsigned int ConfigurationSystem::get_coverage_min_chunk_word_count() const {
    std::string value = const_cast<ConfigurationSystem*>(this)->get_or_default("COVERAGE", "min_chunk_word_count", "4");
    try {
        return std::stoul(value);
    } catch (...) {
        return 4u; // Default fallback
    }
}

float ConfigurationSystem::get_coverage_numeric_boost() const {
    std::string value = const_cast<ConfigurationSystem*>(this)->get_or_default("COVERAGE", "numeric_boost", "0.10");
    try {
        return std::stof(value);
    } catch (...) {
        return 0.10f; // Default fallback
    }
}

float ConfigurationSystem::get_coverage_numeric_mismatch_penalty() const {
    std::string value = const_cast<ConfigurationSystem*>(this)->get_or_default("COVERAGE", "numeric_mismatch_penalty", "0.15");
    try {
        return std::stof(value);
    } catch (...) {
        return 0.15f; // Default fallback
    }
}

float ConfigurationSystem::get_coverage_temporal_penalty_weight() const {
    std::string value = const_cast<ConfigurationSystem*>(this)->get_or_default("COVERAGE", "temporal_penalty_weight", "0.05");
    try {
        return std::stof(value);
    } catch (...) {
        return 0.05f; // Default fallback
    }
}

unsigned int ConfigurationSystem::get_coverage_short_chunk_word_threshold() const {
    std::string value = const_cast<ConfigurationSystem*>(this)->get_or_default("COVERAGE", "short_chunk_word_threshold", "10");
    try {
        return std::stoul(value);
    } catch (...) {
        return 10u; // Default fallback
    }
}

float ConfigurationSystem::get_coverage_lexical_mismatch_scaling_factor() const {
    std::string value = const_cast<ConfigurationSystem*>(this)->get_or_default("COVERAGE", "lexical_mismatch_scaling_factor", "0.60");
    try {
        return std::stof(value);
    } catch (...) {
        return 0.60f; // Default fallback
    }
}

float ConfigurationSystem::get_importance_weight_bold() const {
    std::string value = const_cast<ConfigurationSystem*>(this)->get_or_default("COVERAGE", "importance_weight_bold", "1.5");
    try {
        return std::stof(value);
    } catch (...) {
        return 1.5f; // Default fallback
    }
}

float ConfigurationSystem::get_importance_weight_italic() const {
    std::string value = const_cast<ConfigurationSystem*>(this)->get_or_default("COVERAGE", "importance_weight_italic", "1.2");
    try {
        return std::stof(value);
    } catch (...) {
        return 1.2f; // Default fallback
    }
}

float ConfigurationSystem::get_importance_weight_underline() const {
    std::string value = const_cast<ConfigurationSystem*>(this)->get_or_default("COVERAGE", "importance_weight_underline", "1.3");
    try {
        return std::stof(value);
    } catch (...) {
        return 1.3f; // Default fallback
    }
}

float ConfigurationSystem::get_importance_weight_bg_color() const {
    std::string value = const_cast<ConfigurationSystem*>(this)->get_or_default("COVERAGE", "importance_weight_bg_color", "1.4");
    try {
        return std::stof(value);
    } catch (...) {
        return 1.4f; // Default fallback
    }
}

unsigned int ConfigurationSystem::get_max_pdf_page_count() const {
    std::string value = const_cast<ConfigurationSystem*>(this)->get_or_default("USER_LIMITS", "max_pdf_page_count", "100");
    try {
        return std::stoul(value);
    } catch (...) {
        return 100u; // Default fallback
    }
}

unsigned int ConfigurationSystem::get_max_recording_duration_minutes() const {
    std::string value = const_cast<ConfigurationSystem*>(this)->get_or_default("USER_LIMITS", "max_recording_duration_minutes", "30");
    try {
        return std::stoul(value);
    } catch (...) {
        return 30u; // Default fallback
    }
}

rst_code_e ConfigurationSystem::set_value(const std::string& section, const std::string& field, const std::string& value) {
    return set(section, field, value);
}
