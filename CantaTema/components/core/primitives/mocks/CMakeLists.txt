include(library_install_mock)

create_mock_library(
    TARGET
        "mock_tool_paths"
    SRCS
        src/mock_tool_paths.cpp
    INC_PUBLIC
    INC_PRIVATE
        gtest
        gmock
)