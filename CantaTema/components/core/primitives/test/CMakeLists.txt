include(library_install_test)

create_test_exec(
    TARGET
        "t_primitives"
    SRCS
        t_primitives_main.cxx
        t_tool_paths.cxx
        t_utils_functions.cxx
        t_utils_thread_pool.cxx
        t_definitions.cxx
        t_user_configuration.cxx
        t_language_profile.cxx
        t_domain_profile.cxx
        t_analysis_task.cxx
        t_tag.cxx
    INC_PRIVATE
        CORE::PRIMITIVES
)
