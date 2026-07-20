#####################################################################################################
# PROJECT INFORMATION
#####################################################################################################

cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++23")

# Define UNICODE and _UNICODE for wide-character support on Windows
if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE)
    # Ensure wchar_t support is enabled
    add_definitions(-D_WCHAR_T)
    if(MSVC)
        add_compile_options(/utf-8)
    endif()
endif()

set(PROJ_MAJOR_VERSION 1)
set(PROJ_MINOR_VERSION 0)
set(PROJ_PATCH_VERSION 0)
set(PROJ_VERSION
  ${PROJ_MAJOR_VERSION}.${PROJ_MINOR_VERSION}.${PROJ_PATCH_VERSION})

project(
  CantaTema
  DESCRIPTION "CantaTema - Train your voice. Master your topics."
  VERSION ${PROJ_VERSION}
  LANGUAGES CXX C
)

# SDL3 uses Objective-C (.m) for Cocoa/Metal backends on Apple platforms.
# The language must be enabled at the top level so CMake initialises its
# compiler rules before FetchContent brings in SDL3.
if(APPLE)
  enable_language(OBJC)
endif()

option(ENABLE_COVERAGE "Enable code coverage compilation flags" ON)
if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(--coverage)
        add_link_options(--coverage)
        message(STATUS "Code coverage instrumentation enabled")
        find_program(GCOV_PATH NAMES gcov gcov.exe)
        if(GCOV_PATH)
            set(CTEST_COVERAGE_COMMAND "${GCOV_PATH}")
            message(STATUS "CTest coverage command set to: ${GCOV_PATH}")
            file(WRITE "${CMAKE_BINARY_DIR}/CTestCustom.cmake" "
                set(CTEST_CUSTOM_COVERAGE_EXCLUDE
                    \"/usr/.*\"
                    \".*/_deps/.*\"
                    \".*/test/.*\"
                    \".*/mocks/.*\"
                    \".*/build/.*\"
                )
            ")
        else()
            message(WARNING "gcov tool not found. CTest coverage reports might fail to generate.")
        endif()
    else()
        message(WARNING "Code coverage is only supported for GCC/Clang. Ignored. Compiler ID: ${CMAKE_CXX_COMPILER_ID}")
    endif()
endif()

set(CMAKE_MAIN_PROJECT_NAME ${PROJECT_NAME})

add_compile_definitions(MAIN_PROJECT_NAME=${PROJECT_NAME})

#####################################################################################################
# PROJECT CONFIGURATION
#####################################################################################################

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Installation Directory" FORCE)
endif()

# Offer the user the choice of overriding the installation directories
set(INSTALL_LIB_DIR lib CACHE PATH "Installation directory for libraries")
set(INSTALL_BIN_DIR bin CACHE PATH "Installation directory for executables")
set(INSTALL_INCLUDE_DIR include CACHE PATH "Installation directory for header files")
set(INSTALL_CMAKE_DIR "lib/cmake" CACHE PATH "Installation directory for CMake files")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
     
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

#####################################################################################################
# PROJECT COMPILATION
#####################################################################################################

# define our sub projects that will not require to use the find_package
# functionality
set(SUBPROJECTS utf8cpp)

# find_package override
if(POLICY CMP0057)
  cmake_policy(SET CMP0057 NEW)
endif()
# this is disencourage due to: https://crascit.com/2018/09/14/do-not-redefine-cmake-commands/
macro(find_package)
  if(NOT "${ARGV0}" IN_LIST SUBPROJECTS)
    _find_package(${ARGV})
  endif()
endmacro()

enable_testing()

# Remove -fmodule-mapper from CMAKE_CXX_FLAGS
if(CMAKE_CXX_FLAGS MATCHES "-fmodule-mapper")
  string(REPLACE "-fmodule-mapper" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif()

# Remove -fmodule-mapper from CMAKE_C_FLAGS
if(CMAKE_C_FLAGS MATCHES "-fmodule-mapper")
  string(REPLACE "-fmodule-mapper" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
endif()

include_directories(${PROJECT_BINARY_DIR}/include)

################################
# OS & Architecture Detection
################################

if(WIN32)
  set(OS_TAG "win")
  # For standalone Windows binary, link runtime statically (/MT)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  if(MSVC)
    add_compile_options(/utf-8) # Force UTF-8 for Spanish char support
  else()
    add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)
  endif()
elseif(APPLE)
  set(OS_TAG "mac")
elseif(UNIX)
  set(OS_TAG "linux")
endif()

# Detect Arch (x64 or arm64)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "arm|aarch64")
  set(ARCH_TAG "arm64")
else()
  set(ARCH_TAG "x64")
endif()

message(STATUS "Configuring for: ${OS_TAG}-${ARCH_TAG}")

#####################################################################################################
# EXTERNAL DEPENDENCIES INSTALLATION
#####################################################################################################

set(BUILD_TESTING ON CACHE BOOL "" FORCE)
include(CTest)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
include(FetchContent)
# include(ExternalProject)
# include(CMakePrintHelpers)
# include(GenerateExportHeader)

set(CPM_DOWNLOAD_LOCATION ${CMAKE_BINARY_DIR}/CPM.cmake)

file(DOWNLOAD
  https://github.com/cpm-cmake/CPM.cmake/releases/latest/download/cpm.cmake
  ${CPM_DOWNLOAD_LOCATION})

include(${CPM_DOWNLOAD_LOCATION})
        
################################
# GOOGLE TEST
################################

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.17.0
)
set(gtest_force_shared_crt OFF CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

################################
# FMT
################################

FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 12.1.0
  OVERRIDE_FIND_PACKAGE
)
set(FMT_INSTALL OFF CACHE BOOL "" FORCE)
set(FMT_TEST OFF CACHE BOOL "" FORCE)
set(FMT_DOC OFF CACHE BOOL "" FORCE)
if(WIN32)
  set(FMT_USE_WINDOWS_H ON)
endif()
FetchContent_MakeAvailable(fmt)

################################
# SPDLOG
################################

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.16.0
  OVERRIDE_FIND_PACKAGE
)
set(SPDLOG_INSTALL OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(SPDLOG_COMPILED_LIB ON)
set(SPDLOG_BUILD_EXAMPLES OFF) # Example: Disable building examples
set(SPDLOG_BUILD_TESTS OFF) # Example: Disable building tests
set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(spdlog)

################################
# CLI lib
################################

FetchContent_Declare(
  cli
  GIT_REPOSITORY https://github.com/daniele77/cli.git
  GIT_TAG v2.2.0
)
set(CLI_BuildTests OFF CACHE BOOL "" FORCE)
set(CLI_BuildExamples OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(cli)

include(add_asio_library)
target_link_libraries(cli INTERFACE asio)

################################
# SQLITE
################################

include(add_sqlite3_library)

################################
# opus
################################

FetchContent_Declare(
    libopus
    GIT_REPOSITORY https://github.com/xiph/opus.git
    GIT_TAG        v1.5.2  # Use a specific version tag for stability
)
set(OPUS_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(OPUS_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(OPUS_BUILD_SHARED_LIBRARY OFF CACHE BOOL "" FORCE)
set(OPUS_INSTALL_PKG_CONFIG_MODULE OFF CACHE BOOL "" FORCE)
set(OPUS_INSTALL_CMAKE_CONFIG_MODULE OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(libopus)


################################
# utf8cpp (needed for taglib)
################################

FetchContent_Declare(
  utf8cpp
  GIT_REPOSITORY https://github.com/nemtrif/utfcpp.git
  GIT_TAG v4.0.9
)
set(UTF8_TESTS OFF CACHE BOOL "" FORCE)
set(UTF8_SAMPLES OFF CACHE BOOL "" FORCE)
set(UTF8_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(utf8cpp)
# Manually set utf8cpp_FOUND to prevent TagLib from trying to build it again
# and causing a target name collision.
set(utf8cpp_FOUND TRUE CACHE BOOL "" FORCE)

################################
# taglib
################################

FetchContent_Declare(
  taglib
  GIT_REPOSITORY https://github.com/taglib/taglib.git
  GIT_TAG v2.1.1
)
# Disable optional utf8cpp dependency
set(WITH_APE OFF CACHE BOOL "" FORCE)
set(WITH_ASF OFF CACHE BOOL "" FORCE)
set(WITH_DSF OFF CACHE BOOL "" FORCE)
set(WITH_MATROSKA OFF CACHE BOOL "" FORCE)
set(WITH_MOD OFF CACHE BOOL "" FORCE)
set(WITH_MP4 OFF CACHE BOOL "" FORCE)
set(WITH_SHORTEN OFF CACHE BOOL "" FORCE)
set(WITH_TRUEAUDIO OFF CACHE BOOL "" FORCE)
set(WITH_ZLIB OFF CACHE BOOL "" FORCE)
set(TAGLIB_INSTALL OFF CACHE BOOL "" FORCE)
set(TAGLIB_BUILD_BINDINGS OFF CACHE BOOL "" FORCE)
set(TAGLIB_WITH_ZLIB OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(taglib)

################################
# SDL3
################################

add_compile_definitions(SDL_MAIN_HANDLED)

set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
set(SDLMIXER_INSTALL OFF CACHE BOOL "" FORCE)
set(SDLMIXER_DEPS_SHARED OFF)
set(SDLMIXER_VENDORED ON)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_VIDEO ON CACHE BOOL "" FORCE)
set(SDL_JOYSTICK OFF CACHE BOOL "" FORCE)
set(SDL_HAPTIC OFF CACHE BOOL "" FORCE)
set(SDL_HIDAPI OFF CACHE BOOL "" FORCE)
set(SDL_POWER OFF CACHE BOOL "" FORCE)
set(SDL_SENSOR OFF CACHE BOOL "" FORCE)
set(SDL_CAMERA OFF CACHE BOOL "" FORCE)
set(SDL_DIALOG ON CACHE BOOL "" FORCE)
set(SDL_RENDER OFF CACHE BOOL "" FORCE)
set(SDL_GPU OFF CACHE BOOL "" FORCE)
set(SDL_OPENGL OFF CACHE BOOL "" FORCE)
set(SDL_OPENGLES OFF CACHE BOOL "" FORCE)
set(SDL_VULKAN OFF CACHE BOOL "" FORCE)
set(SDL_METAL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  SDL3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-3.4.0
  OVERRIDE_FIND_PACKAGE
)
set(SDL_TEST OFF CACHE BOOL "Build the SDL3 test programs" FORCE)
FetchContent_MakeAvailable(SDL3)


################################
# simpleini
################################

FetchContent_Declare(
  simpleini
  GIT_REPOSITORY https://github.com/brofield/simpleini.git
  GIT_TAG v4.25
)
FetchContent_MakeAvailable(simpleini)

if(NOT TARGET simpleini)
  add_library(simpleini INTERFACE)
  target_include_directories(simpleini INTERFACE
    $<BUILD_INTERFACE:${simpleini_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
  )
  install(FILES ${simpleini_SOURCE_DIR}/SimpleIni.h DESTINATION ${INSTALL_INCLUDE_DIR})
  install(TARGETS simpleini EXPORT configurationTargets)
endif()

################################
# libcurl (Minimal / HTTP Only)
################################

# Optimization: Disable unused protocols and tools to save space
set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(HTTP_ONLY ON CACHE BOOL "" FORCE) # Disables FTP, LDAP, POP3, IMAP, etc.
set(CURL_ENABLE_SSL ON CACHE BOOL "" FORCE)
set(CURL_DISABLE_VERBOSE_STRINGS ON CACHE BOOL "" FORCE) # Strips debug strings

if(WIN32)
  set(CURL_USE_SCHANNEL ON CACHE BOOL "" FORCE) # Use Windows Native SSL (Small size)
  set(CURL_USE_OPENSSL OFF CACHE BOOL "" FORCE)
elseif(APPLE)
  set(CURL_USE_SECTRANSP ON CACHE BOOL "" FORCE) # Use Apple Native SSL
  set(CURL_USE_OPENSSL OFF CACHE BOOL "" FORCE)
  set(CURL_CA_FALLBACK ON CACHE BOOL "" FORCE)
endif()

FetchContent_Declare(
  curl
  GIT_REPOSITORY https://github.com/curl/curl.git
  GIT_TAG curl-8_18_0
)
FetchContent_MakeAvailable(curl)

################################
# mupdf
################################

include(add_mupdf_library)


################################
# whisper
################################

set(WHISPER_BUILD_TESTS OFF CACHE BOOL "Disable whisper tests" FORCE)
set(WHISPER_BUILD_EXAMPLES OFF CACHE BOOL "Disable whisper examples" FORCE)

if(APPLE)
  set(WHISPER_METAL ON CACHE BOOL "Enable CoreML/Metal support" FORCE)
elseif(WIN32 OR UNIX)
  # Enable Vulkan for Whisper acceleration on Windows/Linux (Requires Vulkan SDK)
  set(WHISPER_VULKAN ON CACHE BOOL "Enable Whisper Vulkan support" FORCE)
endif()

FetchContent_Declare(
    whisper
    GIT_REPOSITORY https://github.com/ggerganov/whisper.cpp.git
    GIT_TAG        v1.8.3
    OVERRIDE_FIND_PACKAGE 
)
FetchContent_MakeAvailable(whisper)

#####################################################################################################
# PROJECT RULES
#####################################################################################################

# --- auto-ignore build directory. Create file if do not exists into the build folder
if(NOT EXISTS ${PROJECT_BINARY_DIR}/.gitignore)
  file(WRITE ${PROJECT_BINARY_DIR}/.gitignore "*")
endif()

#####################################################################################################
# PROJECT SOURCES
#####################################################################################################

add_subdirectory(CantaTema)
