cmake_minimum_required(VERSION 3.20)
project(cantatema_bridge)

add_library(cantatema_bridge SHARED
    src/cantatema_c_api.cpp
)

target_compile_definitions(cantatema_bridge
    PRIVATE
        CANTATEMA_EXPORTS
)

target_include_directories(cantatema_bridge
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(cantatema_bridge
    PRIVATE
        BUSINESS::SESSION
        CORE::PRIMITIVES
        CORE::CONFIGURATION
        INFRASTRUCTURE::DATABASE
        nlohmann_json::nlohmann_json
)

if(ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(cantatema_bridge PUBLIC --coverage)
    target_link_options(cantatema_bridge PUBLIC --coverage)
endif()

# Set runtime output directories so the DLL is alongside executables
set_target_properties(cantatema_bridge PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

add_library(COMPONENTS::C_API ALIAS cantatema_bridge)

add_subdirectory(test)
