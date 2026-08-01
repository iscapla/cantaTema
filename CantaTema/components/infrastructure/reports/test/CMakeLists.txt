include(library_install_test)

create_test_exec(
    TARGET
        "t_reports"
    SRCS
        t_reports_main.cxx
        t_reports.cxx
    INC_PRIVATE
        CORE::PRIMITIVES
        INFRASTRUCTURE::REPORTS
)
