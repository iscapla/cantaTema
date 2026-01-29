include(library_install_infra)

create_infra_library(
    TARGET
    "file_handler"
    SRCS
    src/file_handler.cpp
    src/text_handler.cpp
    INC_PUBLIC
    INC_PRIVATE
    tinyfiledialogs
    CORE::PRIMITIVES
    CORE::CONFIGURATION
)

add_subdirectory(test)
