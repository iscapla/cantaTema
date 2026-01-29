include(library_install_test)

create_test_exec(
    TARGET
        "t_sound_system"
    SRCS
        t_sound_system_main.cxx
        t_sound_system.cxx
    INC_PRIVATE
        CORE::PRIMITIVES
        INFRASTRUCTURE::SOUND_SYSTEM
)