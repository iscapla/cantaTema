# ============================================================================
# SQLite3 Multiple Ciphers – Amalgamation Integration
# ============================================================================
# Provides:
#   - A single library target: sqlite3
#   - Built from sqlite3mc_amalgamation.c
#   - PIC-enabled for safe linkage into shared libraries
#
# Usage:
#   include(cmake/SQLite3MultipleCiphers.cmake)
#   target_link_libraries(my_target PRIVATE sqlite3)
# ============================================================================

include(FetchContent)

# ----------------------------------------------------------------------------
# Version configuration
# ----------------------------------------------------------------------------

# SQLite3 Multiple Ciphers v2.4.0 (based on SQLite 3.53.4)
set(SQLITE_MC_VER "2.4.0")
set(SQLITE_VER "3.53.4")

set(SQLITE_MC_URL
    "https://github.com/utelle/SQLite3MultipleCiphers/releases/download/v${SQLITE_MC_VER}/sqlite3mc-${SQLITE_MC_VER}-sqlite-${SQLITE_VER}-amalgamation.zip"
)

# ----------------------------------------------------------------------------
# Fetch source archive
# ----------------------------------------------------------------------------

FetchContent_Declare(
    sqlite3mc_src
    URL ${SQLITE_MC_URL}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_GetProperties(sqlite3mc_src)
if(NOT sqlite3mc_src_POPULATED)
    FetchContent_Populate(sqlite3mc_src)
endif()

set(SQLITE3MC_SRC_DIR "${sqlite3mc_src_SOURCE_DIR}")

# ----------------------------------------------------------------------------
# SQLite3 Multiple Ciphers amalgamation files
# ----------------------------------------------------------------------------

set(SQLITE3_AMALGAMATION_C
    "${SQLITE3MC_SRC_DIR}/sqlite3mc_amalgamation.c"
)
set(SQLITE3_AMALGAMATION_H
    "${SQLITE3MC_SRC_DIR}/sqlite3mc_amalgamation.h"
)

if(NOT EXISTS "${SQLITE3_AMALGAMATION_C}")
    message(FATAL_ERROR
        "SQLite3 Multiple Ciphers amalgamation source not found:\n"
        "  ${SQLITE3_AMALGAMATION_C}"
    )
endif()

if(NOT EXISTS "${SQLITE3_AMALGAMATION_H}")
    message(FATAL_ERROR
        "SQLite3 Multiple Ciphers amalgamation header not found:\n"
        "  ${SQLITE3_AMALGAMATION_H}"
    )
endif()

# ----------------------------------------------------------------------------
# Build SQLite library (cipher-enabled, PIC-safe)
# ----------------------------------------------------------------------------

add_library(sqlite3
    "${SQLITE3_AMALGAMATION_C}"
)

# REQUIRED to allow linking into shared libraries
set_target_properties(sqlite3 PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

target_include_directories(sqlite3
    PUBLIC
    $<BUILD_INTERFACE:${SQLITE3MC_SRC_DIR}>
    $<INSTALL_INTERFACE:include>
)

# ----------------------------------------------------------------------------
# Compile definitions
# ----------------------------------------------------------------------------

target_compile_definitions(sqlite3
    PRIVATE
    SQLITE_HAS_CODEC=1
    SQLITE_ENABLE_FTS5
    SQLITE_ENABLE_JSON1
    SQLITE_ENABLE_RTREE
    SQLITE_THREADSAFE=1
)

# ----------------------------------------------------------------------------
# Platform-specific linkage
# ----------------------------------------------------------------------------

if(UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    target_link_libraries(sqlite3 PRIVATE m Threads::Threads ${CMAKE_DL_LIBS})
endif()

# ----------------------------------------------------------------------------
# Installation
# ----------------------------------------------------------------------------

install(TARGETS sqlite3
    EXPORT databaseTargets
    ARCHIVE DESTINATION ${CMAKE_BINARY_DIR}/lib
    LIBRARY DESTINATION ${CMAKE_BINARY_DIR}/lib
    RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/bin
)
install(FILES "${SQLITE3_AMALGAMATION_H}" DESTINATION ${CMAKE_BINARY_DIR}/include)

message(STATUS "SQLite3 Multiple Ciphers ${SQLITE_MC_VER} configured")
message(STATUS "Target provided: sqlite3 (PIC enabled)")
