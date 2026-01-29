#pragma once

#include <gmock/gmock.h>
#include <filesystem>
#include "primitives/tool_paths.hpp"

class MockToolPath {
public:
    MOCK_METHOD(ToolPath::OSType, get_os_type, (), ());
    MOCK_METHOD(std::filesystem::path, get_path_for_files, (), ());
    MOCK_METHOD(std::filesystem::path, get_path_for_database, (), ());
    MOCK_METHOD(std::filesystem::path, get_path_for_system_config, (), ());
    MOCK_METHOD(std::filesystem::path, get_path_for_logs, (), ());
};

// Global pointer to the mock instance.
// Set this in your test setup (e.g., SetUp()) and clear it in teardown (e.g., TearDown()).
extern MockToolPath* g_mockToolPath;
