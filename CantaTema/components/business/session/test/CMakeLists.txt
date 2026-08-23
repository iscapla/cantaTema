include(library_install_test)

create_test_exec(
    TARGET
        "t_session"
    SRCS
        t_session_main.cxx
        t_session.cxx

    INC_PRIVATE
        CORE::PRIMITIVES
        CORE::CONFIGURATION
        BUSINESS::OPERATIONS
        BUSINESS::SESSION
        INFRASTRUCTURE::DATABASE
        INFRASTRUCTURE::SPEECH_RECOGNITION
)

