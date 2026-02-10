include(library_install_infra)

create_infra_library(
    TARGET
    "file_handler"
    SRCS
    src/file_handler.cpp
    src/text_handler.cpp
    src/sound_handler.cpp
    INC_PUBLIC
    INC_PRIVATE
    tag #taglib
    tinyfiledialogs
    CORE::PRIMITIVES
    CORE::CONFIGURATION
)

# Fron documentation: https://taglib.org/api/
# TagLib installs a CMake configuration and a taglib-config and pkg-config file to make it easier to integrate into various build systems.
# Note that TagLib's include install directory must be included in the header include path. Simply adding <taglib/tag.h> will not work.
get_target_property(TAG_INCLUDE_DIRS tag INCLUDE_DIRECTORIES)
target_include_directories(_infra_FILE_HANDLER PRIVATE
    $<BUILD_INTERFACE:${taglib_SOURCE_DIR}>
    $<BUILD_INTERFACE:${taglib_SOURCE_DIR}>/taglib
    ${TAG_INCLUDE_DIRS}
)

add_subdirectory(test)
