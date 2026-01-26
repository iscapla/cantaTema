include(library_install_infra)

create_infra_library(
    TARGET
    "primitives"
    SRCS
    src/utils_logger.cpp
    src/utils_thread_pool.cpp
    src/user.cpp
    src/subject.cpp
    src/category.cpp
    src/tool_paths.cpp
    INC_PUBLIC
    spdlog
    INC_PRIVATE
    SDL3::SDL3
)

add_subdirectory(test)
