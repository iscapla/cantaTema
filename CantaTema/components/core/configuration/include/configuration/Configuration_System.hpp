
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