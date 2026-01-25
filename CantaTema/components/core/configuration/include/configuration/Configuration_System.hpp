
#ifndef CONFIGURATION_SYSTEM_HPP
#define CONFIGURATION_SYSTEM_HPP

#include <string>

#include "configuration/IConfigurationBase.hpp"

class Configuration_System : public IConfigurationBase {
public:
    static Configuration_System& getInstance();

    static const unsigned int MAX_EXTENSIONS_LENGTH = 20;
    static const unsigned int MAX_EXTENSIONS_COUNT = 5;

    Configuration_System(const Configuration_System&) = delete;
    Configuration_System& operator=(const Configuration_System&) = delete;

    /**
     * @brief Retrieves the allowed text file extensions from the configuration.
     * 
     * @param patterns Array of character pointers to be populated with extension strings.
     * @param max_patterns The maximum number of patterns the array can hold.
     * @param max_extension_size The maximum size of each extension string.
     * @return int The number of extensions actually retrieved and stored in patterns.
     */
    int get_text_files_extensions_allowed(char patterns[MAX_EXTENSIONS_COUNT][MAX_EXTENSIONS_LENGTH]);

private:
    Configuration_System();
    ~Configuration_System();

    inline static const std::string config_file_name = "system.ini";

};

#endif // CONFIGURATION_SYSTEM_HPP