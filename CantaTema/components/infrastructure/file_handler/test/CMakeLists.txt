include(library_install_test)

create_test_exec(
    TARGET
        "t_file_handler"
    SRCS
        t_file_handler_main.cxx
        t_file_handler.cxx
        t_text_handler.cxx
        t_sound_handler.cxx
        t_extension_type_pdf.cxx
        t_text_chunk_extractor.cxx
        t_text_conversion_coverage.cxx
    INC_PRIVATE
        CORE::PRIMITIVES
        CORE::CONFIGURATION
        INFRASTRUCTURE::FILE_HANDLER
)

add_custom_command(TARGET t_file_handler POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../example_data
    $<TARGET_FILE_DIR:t_file_handler>/example_data
    COMMENT "Copying example_data to t_file_handler binary directory"
)