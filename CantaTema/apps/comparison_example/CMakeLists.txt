project(
    ComparisonExample
    DESCRIPTION "CantaTema - Comparison Example Developer Binary"
    VERSION ${PROJ_VERSION}
    LANGUAGES CXX
)

message(STATUS "Building ${PROJECT_NAME}...")

add_executable(${PROJECT_NAME} main_comparison_example.cpp)

if(MINGW OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_options(${PROJECT_NAME} PRIVATE -Wl,--allow-multiple-definition)
endif()

# link
target_link_libraries(${PROJECT_NAME} PRIVATE
    CORE::PRIMITIVES
    CORE::CONFIGURATION
    CORE::MODELS
    INFRASTRUCTURE::SOUND_SYSTEM
    INFRASTRUCTURE::FILE_HANDLER
    INFRASTRUCTURE::SPEECH_RECOGNITION
    INFRASTRUCTURE::EMBEDDINGS
    INFRASTRUCTURE::SIMILARITY
    INFRASTRUCTURE::REPORTS
    whisper
    llama
)

# install information
install(TARGETS ${PROJECT_NAME} DESTINATION bin/${PROJECT_NAME})

# Set the runtime output directory for executable
set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${PROJECT_NAME})

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/../../example_data
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/example_data
    COMMENT "Copying example_data to ${PROJECT_NAME} binary directory"
)
