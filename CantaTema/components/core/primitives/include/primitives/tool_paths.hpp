#ifndef TOOL_PATH_HPP
#define TOOL_PATH_HPP

#include <string>
#include <filesystem>
#include <cstdlib>

// OS Detection Macros
#if defined(_WIN32)
    #define TOOLPATH_OS_WINDOWS
#elif defined(__ANDROID__)
    #define TOOLPATH_OS_ANDROID
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE
        #define TOOLPATH_OS_IOS
    #else
        #define TOOLPATH_OS_MACOS
    #endif
#elif defined(__linux__)
    #define TOOLPATH_OS_LINUX
#endif

class ToolPath {
public:
    enum class OSType {
        Windows,
        Linux,
        MacOS,
        IOS,
        Android,
        Unknown
    };

    /**
     * @brief Detects the current Operating System.
     * 
     * @return OSType The detected OS.
     */
    static OSType get_os_type(void);

    /**
     * @brief Returns the path where application files should be stored.
     * Creates the directory if it does not exist.
     * 
     * @return std::filesystem::path Absolute path to the files directory.
     */
    static std::filesystem::path get_path_for_files(void);

    /**
     * @brief Returns the path where the database should be stored.
     * Creates the directory if it does not exist.
     * 
     * @return std::filesystem::path Absolute path to the database folder.
     */
    static std::filesystem::path get_path_for_database(void);

    /**
     * @brief Returns the path where the main system configuration files should be stored.
     * Creates the directory if it does not exist.
     * 
     * @return std::filesystem::path Absolute path to the config folder.
     */
    static std::filesystem::path get_path_for_system_config(void);

    /**
     * @brief Returns the path where the logs files should be stored.
     * Creates the directory if it does not exist.
     * 
     * @return std::filesystem::path Absolute path to the logs folder.
     */
    static std::filesystem::path get_path_for_logs(void);

    /**
     * @brief Returns the path where whisper models should be stored.
     * Creates the directory if it does not exist.
     * 
     * @return std::filesystem::path Absolute path to the models/whisper folder.
     */
    static std::filesystem::path get_path_for_models_whisper(void);

    /**
     * @brief Returns the path where subject files should be stored.
     * Creates the directory if it does not exist.
     * 
     * @param user_id 
     * @param subject_id 
     * @return std::filesystem::path 
     */
    static std::filesystem::path get_path_for_subject(unsigned int user_id, unsigned int subject_id);

    /**
     * @brief Returns the path where practice event files should be stored.
     * Creates the directory if it does not exist.
     * 
     * @param user_id 
     * @param subject_id 
     * @return std::filesystem::path 
     */
    static std::filesystem::path get_path_for_practice_event(unsigned int user_id, unsigned int subject_id);

private:
    static void ensure_directory_exists(const std::filesystem::path& path);
    static std::filesystem::path get_base_path(void);

    /**
     * @brief Get the platform specific path for application data using SDL.
     * The directory is created if it does not exist.
     * 
     * @param org_name Organization name
     * @param app_name Application name
     * @return std::string Path with trailing separator
     */
    static std::string get_filesystem_path(const std::string &org_name, const std::string &app_name);
};

#endif // TOOL_PATH_HPP