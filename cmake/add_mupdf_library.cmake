include(FetchContent)

# ============================================================
# 1. Fetch MuPDF (Source only)
# ============================================================

FetchContent_Declare(
    mupdf_src
    GIT_REPOSITORY https://github.com/ArtifexSoftware/mupdf.git
    GIT_TAG        1.27.1
    GIT_SUBMODULES "" # Prevent CMake from handling submodules, we do it manually below
)

FetchContent_GetProperties(mupdf_src)
if(NOT mupdf_src_POPULATED)
    FetchContent_Populate(mupdf_src)
    
    # ============================================================
    # 2. Download ALL submodules
    # ============================================================
    find_package(Git REQUIRED)
    message(STATUS "Updating all MuPDF submodules...")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
        WORKING_DIRECTORY ${mupdf_src_SOURCE_DIR}
        RESULT_VARIABLE git_result
    )
    if(NOT git_result EQUAL 0)
        message(FATAL_ERROR "Failed to update MuPDF submodules")
    endif()
endif()

set(MUPDF_ROOT ${mupdf_src_SOURCE_DIR})

# ============================================================
# 3. Configure Build Command (Run Make)
# ============================================================

set(MUPDF_DEFINITIONS
    # -DFZ_ENABLE_PDF=1
    # -DFZ_ENABLE_ICC=0
    # -DFZ_ENABLE_XPS=0
    # -DFZ_ENABLE_SVG=0
    # -DFZ_ENABLE_HTML=0
    # -DFZ_ENABLE_EPUB=0
    # -DFZ_ENABLE_CBZ=0
    # -DFZ_ENABLE_JPX=0
    # -DFZ_ENABLE_FREETYPE=1
    # -DFZ_ENABLE_HARFBUZZ=1
    # -DFZ_ENABLE_JBIG2DEC=0
    # -DFZ_ENABLE_JPEG=1
    # -DFZ_ENABLE_PNG=1
    # -DFZ_ENABLE_TIFF=1
    # -DFZ_ENABLE_HTML_ENGINE=0
    # -DFZ_ENABLE_MOBI=0
    # -DFZ_ENABLE_FB2=0
    # -DFZ_ENABLE_TXT=0
    # -DFZ_ENABLE_OFFICE=0
    # -DFZ_ENABLE_UNCOMPRESS=0
    # -DFZ_ENABLE_JS=0
    # -DFZ_ENABLE_OCR=0
    # -DFZ_ENABLE_DOCX_OUTPUT=0


    # -DFZ_ENABLE_HYPHEN=0
    # -DFZ_ENABLE_HYPHEN_ALL=0
    # -DFZ_ENABLE_IMG=1
    # -DFZ_ENABLE_OCR_OUTPUT=0
    # -DFZ_ENABLE_ODT_OUTPUT=0
    # -DFZ_ENABLE_BROTLI=0
)
string(REPLACE ";" " " MUPDF_DEFINITIONS_STR "${MUPDF_DEFINITIONS}")

# Prepare definitions for interface export (strip -D prefix)
set(MUPDF_EXPORT_DEFINITIONS "")
foreach(DEF IN LISTS MUPDF_DEFINITIONS)
    string(REGEX REPLACE "^-D" "" CLEAN_DEF "${DEF}")
    list(APPEND MUPDF_EXPORT_DEFINITIONS "${CLEAN_DEF}")
endforeach()

set(MUPDF_MAKE_OPTIONS
    # HAVE_X11=no
    # HAVE_GLFW=no
    # HAVE_GLUT=no
    # HAVE_CURL=no
    # HAVE_OPENSSL=no
    # HAVE_LIBCRYPTO=no
    # HAVE_LCMS2=no
    # HAVE_GUMBO=no
    # HAVE_OPENJPEG=no
    # HAVE_FREETYPE=no
    # HAVE_HARFBUZZ=no
    # HAVE_JBIG2DEC=no
    # HAVE_JPEG=no
    # HAVE_PNG=no
    # HAVE_TIFF=no
    # HAVE_MUJS=no
    # HAVE_LEPTONICA=no
    # HAVE_TESSERACT=no
)

if(MSVC)
    find_program(NMAKE_EXE nmake REQUIRED)
    
    # MuPDF NMakefile builds into platform/win32/<Arch>/<Config>/
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(MP_ARCH "x64")
    else()
        set(MP_ARCH "x86")
    endif()
    
    # We force Release for simplicity as NMake is single-config
    set(MP_CONFIG "Release") 
    
    set(BUILD_CMD ${CMAKE_COMMAND} -E env "CL=${MUPDF_DEFINITIONS_STR}" ${NMAKE_EXE} /f platform/win32/NMakefile)
    
    set(LIB_MUPDF "${MUPDF_ROOT}/platform/win32/${MP_ARCH}/${MP_CONFIG}/libmupdf.lib")
    set(LIB_THIRD "${MUPDF_ROOT}/platform/win32/${MP_ARCH}/${MP_CONFIG}/libthirdparty.lib")
else()
    find_program(MAKE_EXE make REQUIRED)
    
    cmake_host_system_information(RESULT N_CORES QUERY NUMBER_OF_PHYSICAL_CORES)
    set(BUILD_CMD ${MAKE_EXE} -j${N_CORES} "XCFLAGS=-msse4.1 ${MUPDF_DEFINITIONS_STR}" ${MUPDF_MAKE_OPTIONS} extract=no OUT=build/release-cmake)
    
    # Standard Makefile builds into build/release/
    set(LIB_MUPDF "${MUPDF_ROOT}/build/release-cmake/libmupdf.a")
    set(LIB_THIRD "${MUPDF_ROOT}/build/release-cmake/libmupdf-third.a")
endif()

# ============================================================
# 3.5. Handle Reconfiguration (Clean if flags changed)
# ============================================================

# MuPDF's makefiles don't detect changes in environment variables/flags.
# We track the build command and force a clean if it changes.
set(MUPDF_CONFIG_FILE "${CMAKE_BINARY_DIR}/mupdf_build_config.txt")
string(REPLACE ";" " " CURRENT_CONFIG_STR "${BUILD_CMD}")

if(EXISTS "${MUPDF_CONFIG_FILE}")
    file(READ "${MUPDF_CONFIG_FILE}" OLD_CONFIG_STR)
else()
    set(OLD_CONFIG_STR "")
endif()

if(NOT "${CURRENT_CONFIG_STR}" STREQUAL "${OLD_CONFIG_STR}")
    message(STATUS "MuPDF build options changed. Cleaning to ensure correct rebuild...")
    if(MSVC)
        execute_process(COMMAND ${NMAKE_EXE} /f platform/win32/NMakefile clean WORKING_DIRECTORY ${MUPDF_ROOT} OUTPUT_QUIET ERROR_QUIET)
    else()
        execute_process(COMMAND ${MAKE_EXE} clean WORKING_DIRECTORY ${MUPDF_ROOT} OUTPUT_QUIET ERROR_QUIET)
    endif()
    file(WRITE "${MUPDF_CONFIG_FILE}" "${CURRENT_CONFIG_STR}")
endif()

# ============================================================
# 4. Build Target
# ============================================================

add_custom_target(mupdf_make
    COMMAND ${BUILD_CMD}
    WORKING_DIRECTORY ${MUPDF_ROOT}
    COMMENT "Building MuPDF using native make..."
    BYPRODUCTS ${LIB_MUPDF} ${LIB_THIRD}
    VERBATIM
)

# ============================================================
# 5. Import Library
# ============================================================

# Helper to import the thirdparty lib
add_library(mupdf_third STATIC IMPORTED GLOBAL)
add_dependencies(mupdf_third mupdf_make)
set_target_properties(mupdf_third PROPERTIES
    IMPORTED_LOCATION "${LIB_THIRD}"
)

# Main mupdf library
add_library(mupdf STATIC IMPORTED GLOBAL)
add_library(mupdf::mupdf ALIAS mupdf)
add_dependencies(mupdf mupdf_make)

set_target_properties(mupdf PROPERTIES
    IMPORTED_LOCATION "${LIB_MUPDF}"
    INTERFACE_INCLUDE_DIRECTORIES "${MUPDF_ROOT}/include"
    INTERFACE_LINK_LIBRARIES mupdf_third
    INTERFACE_COMPILE_DEFINITIONS "${MUPDF_EXPORT_DEFINITIONS}"
)
