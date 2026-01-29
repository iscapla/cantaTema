#include "mock_tool_paths.hpp"

// Initialize the global mock pointer to nullptr
MockToolPath* g_mockToolPath = nullptr;

ToolPath::OSType ToolPath::get_os_type(void) {
    if (g_mockToolPath) {
        return g_mockToolPath->get_os_type();
    }
    return ToolPath::OSType::Unknown;
}

std::filesystem::path ToolPath::get_path_for_files(void) {
    if (g_mockToolPath) {
        return g_mockToolPath->get_path_for_files();
    }
    return "mock_files_path";
}

std::filesystem::path ToolPath::get_path_for_database(void) {
    if (g_mockToolPath) {
        return g_mockToolPath->get_path_for_database();
    }
    // Return a safe default relative path for tests that don't set a mock
    return "mock_database_path";
}

std::filesystem::path ToolPath::get_path_for_system_config(void) {
    if (g_mockToolPath) {
        return g_mockToolPath->get_path_for_system_config();
    }
    return "mock_config_path";
}

std::filesystem::path ToolPath::get_path_for_logs(void) {
    if (g_mockToolPath) {
        return g_mockToolPath->get_path_for_logs();
    }
    return "mock_logs_path";
}

// Private methods (ensure_directory_exists, get_base_path, get_filesystem_path)
// do not need to be implemented here because:
// 1. They are private.
// 2. We are replacing the public static methods that would normally call them.
// 3. Since our mock implementation above doesn't call them, the linker won't complain 
//    about missing symbols for them.