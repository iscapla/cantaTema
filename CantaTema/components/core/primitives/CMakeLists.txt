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
    INC_PUBLIC
    spdlog
    SDL3::SDL3
    INC_PRIVATE
)

add_subdirectory(test)
