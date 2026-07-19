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

    INC_PRIVATE
        CORE::PRIMITIVES
        BUSINESS::OPERATIONS
        INFRASTRUCTURE::DATABASE
)