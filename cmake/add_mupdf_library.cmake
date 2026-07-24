include(FetchContent)
include(ExternalProject)

if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()

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
    message(STATUS "Updating specific MuPDF submodules...")

    # Only download submodules required by the configuration
    set(MUPDF_SUBMODULES
        thirdparty/freetype
        thirdparty/harfbuzz
        thirdparty/zlib
        thirdparty/libjpeg
        thirdparty/openjpeg
        thirdparty/jbig2dec
        thirdparty/mujs
        thirdparty/lcms2
        thirdparty/gumbo-parser
    )

    execute_process(
        COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive -- ${MUPDF_SUBMODULES}
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
    -DFZ_ENABLE_BARCODE=0
    -DFZ_ENABLE_BROTLI=0
    -DFZ_ENABLE_CBZ=0
    -DFZ_ENABLE_DOCX_OUTPUT=0
    -DFZ_ENABLE_EPUB=0
    -DFZ_ENABLE_FB2=0
    -DFZ_ENABLE_FREETYPE=1
    -DFZ_ENABLE_HARFBUZZ=1
    -DFZ_ENABLE_HTML=0
    -DFZ_ENABLE_HTML_ENGINE=0
    -DFZ_ENABLE_HYPHEN=0
    -DFZ_ENABLE_HYPHEN_ALL=0
    -DFZ_ENABLE_ICC=0
    -DFZ_ENABLE_IMG=0
    -DFZ_ENABLE_JBIG2DEC=0
    -DFZ_ENABLE_JPEG=0
    -DFZ_ENABLE_JPX=0
    -DFZ_ENABLE_JS=0
    -DFZ_ENABLE_MOBI=0
    -DFZ_ENABLE_OCR=0
    -DFZ_ENABLE_OCR_OUTPUT=0
    -DFZ_ENABLE_ODT_OUTPUT=0
    -DFZ_ENABLE_OFFICE=0
    -DFZ_ENABLE_PDF=1
    -DFZ_ENABLE_PNG=0
    -DFZ_ENABLE_SPOT_RENDERING=0
    -DFZ_ENABLE_SVG=0
    -DFZ_ENABLE_TIFF=0
    -DFZ_ENABLE_TXT=0
    -DFZ_ENABLE_UNCOMPRESS=0
    -DFZ_ENABLE_XPS=0
    -DFZ_PLOTTERS_CMYK=0
    -DFZ_PLOTTERS_G=0
    -DFZ_PLOTTERS_N=0
    -DFZ_PLOTTERS_RGB=0
)
string(REPLACE ";" " " MUPDF_DEFINITIONS_STR "${MUPDF_DEFINITIONS}")

# Prepare definitions for interface export (strip -D prefix)
set(MUPDF_EXPORT_DEFINITIONS "")
foreach(DEF IN LISTS MUPDF_DEFINITIONS)
    string(REGEX REPLACE "^-D" "" CLEAN_DEF "${DEF}")
    list(APPEND MUPDF_EXPORT_DEFINITIONS "${CLEAN_DEF}")
endforeach()

set(MUPDF_MAKE_OPTIONS
    "CC=${CMAKE_C_COMPILER}"
    "CXX=${CMAKE_CXX_COMPILER}"
    HAVE_CURL=no
    HAVE_FREETYPE=no
    HAVE_GLFW=no
    HAVE_GLUT=no
    HAVE_GUMBO=no
    HAVE_HARFBUZZ=no
    HAVE_JBIG2DEC=no
    HAVE_JPEG=no
    HAVE_JPEGXR=no
    HAVE_LCMS2=no
    HAVE_LEPTONICA=no
    HAVE_LIBARCHIVE=no
    HAVE_LIBCRYPTO=no
    HAVE_LIBDL=no
    HAVE_MUJS=no
    HAVE_OBJCOPY=no
    HAVE_OPENJPEG=no
    HAVE_OPENSSL=no
    HAVE_PNG=no
    HAVE_PTHREAD=no
    HAVE_SMARTOFFICE=no
    HAVE_SYS_CURL=no
    HAVE_SYS_LEPTONICA=no
    HAVE_SYS_LIBARCHIVE=no
    HAVE_SYS_TESSERACT=no
    HAVE_SYS_ZXINGCPP=no
    HAVE_TESSERACT=no
    HAVE_TIFF=no
    HAVE_X11=no
    HAVE_ZXINGCPP=no
    HAVE_CJK=no
    HAVE_CJK_FULL=no
    HAVE_CJK_SINGLE=no
    "FONT_FLAGS="
    "TOFU_FLAGS="
    FONT_CJK=no
    FONT_SMALL=yes
    TOFU_CJK_LANG=no
    TOFU_CJK_EXT=no
    USE_BROTLI=no
    USE_EXTRACT=no
    USE_LEPTONICA=no
    USE_LIBARCHIVE=no
    USE_MUJS=no
    USE_SYSTEM_BROTLI=no
    USE_SYSTEM_CURL=no
    USE_SYSTEM_FREETYPE=no
    USE_SYSTEM_GLUT=no
    USE_SYSTEM_GUMBO=no
    USE_SYSTEM_JBIG2DEC=no
    USE_SYSTEM_JPEGXR=no
    USE_SYSTEM_LCMS2=no
    USE_SYSTEM_LIBJPEG=no
    USE_SYSTEM_LIBS=no
    USE_SYSTEM_MUJS=no
    USE_SYSTEM_OPENJPEG=no
    USE_SYSTEM_ZLIB=no
    USE_TESSERACT=no
    USE_ZXINGCPP=no
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
    
    set(MUPDF_ARCH_FLAGS "")
    if(NOT CMAKE_SYSTEM_PROCESSOR MATCHES "arm|aarch64|ARM64")
        set(MUPDF_ARCH_FLAGS "-msse4.1")
    endif()

    if(WIN32)
        find_program(SH_EXE sh PATHS C:/msys64/usr/bin C:/msys64/ucrt64/bin "C:/Program Files/Git/bin")
        if(SH_EXE)
            string(REPLACE ";" " " MUPDF_MAKE_OPTIONS_STR "${MUPDF_MAKE_OPTIONS}")
            set(BUILD_CMD ${SH_EXE} -c "export PATH=/usr/bin:/ucrt64/bin:C:/msys64/usr/bin:C:/msys64/ucrt64/bin:\$PATH; ${MAKE_EXE} -j${N_CORES} XCFLAGS=\"${MUPDF_ARCH_FLAGS} ${MUPDF_DEFINITIONS_STR}\" ${MUPDF_MAKE_OPTIONS_STR} extract=no build=release OUT=build/release libs")
        else()
            set(BUILD_CMD ${MAKE_EXE} -j${N_CORES} "XCFLAGS=${MUPDF_ARCH_FLAGS} ${MUPDF_DEFINITIONS_STR}" ${MUPDF_MAKE_OPTIONS} extract=no build=release OUT=build/release libs)
        endif()
    else()
        set(BUILD_CMD ${MAKE_EXE} -j${N_CORES} "XCFLAGS=${MUPDF_ARCH_FLAGS} ${MUPDF_DEFINITIONS_STR}" ${MUPDF_MAKE_OPTIONS} extract=no build=release OUT=build/release libs)
    endif()
    
    # Standard Makefile builds into build/release/
    set(LIB_MUPDF "${MUPDF_ROOT}/build/release/libmupdf.a")
    set(LIB_THIRD "${MUPDF_ROOT}/build/release/libmupdf-third.a")
endif()

# ============================================================
# 4. External Build Target using ExternalProject
# ============================================================

ExternalProject_Add(
    mupdf_ext
    SOURCE_DIR "${MUPDF_ROOT}"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ${BUILD_CMD}
    BUILD_IN_SOURCE 1
    INSTALL_COMMAND ""
    BUILD_BYPRODUCTS "${LIB_MUPDF}" "${LIB_THIRD}"
)

# Maintain alias compatibility
add_custom_target(mupdf_make DEPENDS mupdf_ext)

# ============================================================
# 5. Import Library
# ============================================================

# Helper to import the thirdparty lib
add_library(mupdf_third STATIC IMPORTED GLOBAL)
add_dependencies(mupdf_third mupdf_ext)
set_target_properties(mupdf_third PROPERTIES
    IMPORTED_LOCATION "${LIB_THIRD}"
)

# Main mupdf library
add_library(mupdf STATIC IMPORTED GLOBAL)
add_library(mupdf::mupdf ALIAS mupdf)
add_dependencies(mupdf mupdf_ext)

set_target_properties(mupdf PROPERTIES
    IMPORTED_LOCATION "${LIB_MUPDF}"
    INTERFACE_INCLUDE_DIRECTORIES "${MUPDF_ROOT}/include"
    INTERFACE_LINK_LIBRARIES mupdf_third
    INTERFACE_COMPILE_DEFINITIONS "${MUPDF_EXPORT_DEFINITIONS}"
)
