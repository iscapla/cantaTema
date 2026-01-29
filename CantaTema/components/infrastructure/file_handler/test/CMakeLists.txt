include(library_install_test)

create_test_exec(
    TARGET
        "t_file_handler"
    SRCS
        t_file_handler_main.cxx
        t_file_handler.cxx
        t_text_handler.cxx
    INC_PRIVATE
        CORE::PRIMITIVES
        INFRASTRUCTURE::FILE_HANDLER
)