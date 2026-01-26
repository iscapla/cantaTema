
#include <vector>
#include <sstream>
#include <mutex>
#include <cstring>

#include "primitives/tool_paths.hpp"
#include "primitives/utils_logger.hpp"
#include "configuration/Configuration_System.hpp"

Configuration_System& Configuration_System::getInstance() {
    static Configuration_System instance;
    return instance;
}

Configuration_System::Configuration_System() : IConfigurationBase() {
    std::filesystem::path db_path = ToolPath::get_path_for_system_config() / config_file_name;
    set_file_path(db_path);
    parse();

    set_default_if_not_present("TEXT_FILES", "extensions_allowed", "*.txt\n*.pdf");
    update_values_to_file();
}

Configuration_System::~Configuration_System() {
}

/**
 * @brief Retrieves the allowed text file extensions from the configuration.
 * 
 * @param patterns Array of character pointers to be populated with extension strings.
 * @return int The number of extensions actually retrieved and stored in patterns.
 */
int Configuration_System::get_text_files_extensions_allowed(char patterns[MAX_EXTENSIONS_COUNT][MAX_EXTENSIONS_LENGTH]) const {
    const std::string section = "TEXT_FILES";
    const std::string key = "extensions_allowed";

    std::string value = const_cast<Configuration_System*>(this)->get(section, key);

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
