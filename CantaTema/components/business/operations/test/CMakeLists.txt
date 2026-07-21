include(library_install_test)

create_test_exec(
    TARGET
        "t_operation"
    SRCS
        t_operation_main.cxx
        t_operation_category.cxx
        t_operation_practice_event.cxx
        t_operation_subject.cxx
        t_operation_user_metrics.cxx
        t_operation_user.cxx
        t_operation_coverage.cxx

    INC_PRIVATE
        CORE::PRIMITIVES
        CORE::CONFIGURATION
        CORE::MODELS
        BUSINESS::OPERATIONS
        INFRASTRUCTURE::DATABASE
        INFRASTRUCTURE::FILE_HANDLER
        INFRASTRUCTURE::SPEECH_RECOGNITION
        INFRASTRUCTURE::EMBEDDINGS
        INFRASTRUCTURE::SIMILARITY
)