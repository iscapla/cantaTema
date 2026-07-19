include(library_install_test)

create_test_exec(
    TARGET
        "t_models"
    SRCS
        t_models_main.cxx
        t_manager_whisper.cxx
    INC_PRIVATE
        CORE::PRIMITIVES
        CORE::MODELS
)