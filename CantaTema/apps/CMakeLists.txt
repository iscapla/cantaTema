# Set COMPONENT_PROJECT_NAME based on the directory name
get_filename_component(DIR_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
string(TOUPPER ${DIR_NAME} COMPONENT_PROJECT_NAME)

# Create child targets
add_subdirectory(terminal)


project(
    Terminal_CLI
    DESCRIPTION "CantaTema - Terminal_CLI"
    VERSION ${PROJ_VERSION}
    LANGUAGES CXX
)

# inform of our project
message(STATUS "Building ${PROJECT_NAME}...")

add_executable(${PROJECT_NAME} main_terminal.cpp)

# link
target_link_libraries(${PROJECT_NAME} PRIVATE
    CORE::PRIMITIVES
    CORE::CONFIGURATION
    CORE::PRIMITIVES
    APPS::TERMINAL
    INFRASTRUCTURE::SOUND_SYSTEM
)

# install information
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# Set the runtime output directory for each executable
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
