include(library_install_infra)

create_infra_library(
    TARGET
    "file_handler"
    SRCS
    src/extension_type_pdf.cpp
    src/extension_type_txt.cpp
    src/file_handler.cpp
    src/text_handler.cpp
    src/sound_handler.cpp
    src/text_chunk_extractor.cpp
    INC_PUBLIC
    INC_PRIVATE
    mupdf::mupdf
    tag #taglib
    SDL3::SDL3
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
    $<BUILD_INTERFACE:${mupdf_SOURCE_DIR}/include>
)

add_dependencies(_infra_FILE_HANDLER mupdf_ext)

add_subdirectory(test)
