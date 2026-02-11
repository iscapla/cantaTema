include(library_install_test)

create_test_exec(
    TARGET
        "t_configuration"
    SRCS
        t_configuration_main.cxx
        t_configuration_system.cxx
    INC_PRIVATE
        CORE::PRIMITIVES
        CORE::CONFIGURATION
)