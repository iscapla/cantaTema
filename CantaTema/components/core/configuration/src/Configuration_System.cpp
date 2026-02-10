
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

    std::string value = const_cast<ConfigurationSystem*>(this)->get(section, key);

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
    std::string value = const_cast<ConfigurationSystem*>(this)->get("USER_LIMITS", "max_text_file_size_mb");
    try {
        return std::stoul(value);
    } catch (...) {
        return 10; // Default fallback
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
    std::string value = const_cast<ConfigurationSystem*>(this)->get("USER_LIMITS", "max_sound_file_size_mb");
    try {
        return std::stoul(value);
    } catch (...) {
        return 50; // Default fallback
    }
}

unsigned int ConfigurationSystem::get_user_usage_limit_in_mb(void) const {
    std::string value = const_cast<ConfigurationSystem*>(this)->get("USER_LIMITS", "usage_limit_mb");
    try {
        return std::stoul(value);
    } catch (...) {
        return 128; // Default fallback
    }
}
