include(library_install_infra)

create_infra_library(
    TARGET
    "tool_paths"
    SRCS
    src/tool_paths.cpp
    INC_PUBLIC
    INC_PRIVATE
    SDL3::SDL3
    CORE::LOGGER
)

create_infra_library(
    TARGET
    "logger"
    SRCS
    src/utils_logger.cpp
    INC_PUBLIC
    spdlog
    INC_PRIVATE
    CORE::TOOL_PATHS
)

create_infra_library(
    TARGET
    "primitives"
    SRCS
    src/utils_thread_pool.cpp
    src/user.cpp
    src/user_metrics.cpp
    src/subject.cpp
    src/category.cpp
    INC_PUBLIC
    CORE::LOGGER
    INC_PRIVATE
    CORE::TOOL_PATHS
)

add_subdirectory(mocks)
add_subdirectory(test)
