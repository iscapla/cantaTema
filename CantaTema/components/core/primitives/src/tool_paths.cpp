#include <SDL3/SDL.h>

#include "primitives/utils_logger.hpp"
#include "primitives/tool_paths.hpp"

#ifndef MAIN_PROJECT_NAME
    #define MAIN_PROJECT_NAME CantaTema
#endif

#define XSTR(x) STR(x)
#define STR(x) #x

ToolPath::OSType ToolPath::get_os_type(void) {
    #if defined(TOOLPATH_OS_WINDOWS)
        return OSType::Windows;
    #elif defined(TOOLPATH_OS_ANDROID)
        return OSType::Android;
    #elif defined(TOOLPATH_OS_IOS)
        return OSType::IOS;
    #elif defined(TOOLPATH_OS_MACOS)
        return OSType::MacOS;
    #elif defined(TOOLPATH_OS_LINUX)
        return OSType::Linux;
    #else
        return OSType::Unknown;
    #endif
}

std::filesystem::path ToolPath::get_path_for_files(void) {
    std::filesystem::path path = get_base_path() / "data" / "files";
    ensure_directory_exists(path);
    return path;
}

std::filesystem::path ToolPath::get_path_for_database(void) {
    std::filesystem::path path = get_base_path() / "data";
    ensure_directory_exists(path);
    return path;
}

std::filesystem::path ToolPath::get_path_for_system_config(void) {
    std::filesystem::path path = get_base_path() / "config";
    ensure_directory_exists(path);
    return path;
}

std::filesystem::path ToolPath::get_path_for_logs(void) {
    std::filesystem::path path = get_base_path() / "logs";
    ensure_directory_exists(path);
    return path;
}

std::filesystem::path ToolPath::get_path_for_models_whisper(void) {
    std::filesystem::path path = get_base_path() / "data" / "models" / "whisper";
    ensure_directory_exists(path);
    return path;
}

std::filesystem::path ToolPath::get_path_for_subject(unsigned int user_id, unsigned int subject_id) {
    std::filesystem::path path = get_path_for_files() / std::to_string(user_id) / std::to_string(subject_id);
    ensure_directory_exists(path);
    return path;
}

std::filesystem::path ToolPath::get_path_for_practice_event(unsigned int user_id, unsigned int subject_id) {
    std::filesystem::path path = get_path_for_subject(user_id, subject_id) / "practices";
    ensure_directory_exists(path);
    return path;
}

void ToolPath::ensure_directory_exists(const std::filesystem::path& path) {
    try {
        if (!std::filesystem::exists(path)) {
            std::error_code ec;
            if (!std::filesystem::create_directories(path, ec)) {
                logger->error("Error creating directories for {}: {}", path.string(), ec.message());
            }
        }
    } catch (...) {
        // Silently fail or handle error if logger is available
    }
}

std::string ToolPath::get_filesystem_path(const std::string &org_name, const std::string &app_name)
{
    char *base_path = SDL_GetPrefPath(org_name.c_str(), app_name.c_str());
    if (base_path)
    {
        char buffer[256] = {};
        size_t len = SDL_strlcpy(buffer, base_path, sizeof(buffer));
        if (len > 256){
            logger->error("OS system path too long.");
            buffer[0] = '\0';
        }
        std::string path(buffer);
        SDL_free(base_path);

        if (path.empty())
        {
            logger->error("OS system path error.");
            return "";
        }
        return path;
    }
    return "";
}

std::filesystem::path ToolPath::get_base_path(void) {
#if defined(NDEBUG)
    std::filesystem::path base_path{ToolPath::get_filesystem_path(XSTR(MAIN_PROJECT_NAME), XSTR(MAIN_PROJECT_NAME))};
#else
    std::filesystem::path base_path = std::filesystem::current_path();
#endif

    // #if defined(TOOLPATH_OS_WINDOWS)
    //     #ifdef _MSC_VER
    //         char* buf = nullptr;
    //         size_t sz = 0;
    //         if (_dupenv_s(&buf, &sz, "LOCALAPPDATA") == 0 && buf != nullptr) {
    //             base_path = std::filesystem::path(buf) / "CantaTema";
    //             free(buf);
    //         } else {
    //             base_path = std::filesystem::current_path();
    //         }
    //     #else
    //         const char* app_data = std::getenv("LOCALAPPDATA");
    //         if (app_data) base_path = std::filesystem::path(app_data) / "CantaTema";
    //         else base_path = std::filesystem::current_path();
    //     #endif
    // #elif defined(TOOLPATH_OS_MACOS)
    //     const char* home = std::getenv("HOME");
    //     if (home) base_path = std::filesystem::path(home) / "Library" / "Application Support" / "CantaTema";
    //     else base_path = std::filesystem::current_path();
    // #elif defined(TOOLPATH_OS_LINUX)
    //     const char* xdg_data = std::getenv("XDG_DATA_HOME");
    //     if (xdg_data) base_path = std::filesystem::path(xdg_data) / "CantaTema";
    //     else base_path = std::filesystem::path(std::getenv("HOME")) / ".local" / "share" / "CantaTema";
    // #else
    //     base_path = std::filesystem::current_path();
    // #endif
    return base_path;
}