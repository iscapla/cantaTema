include(library_install_infra)

create_infra_library(
    TARGET
    "sound_system"
    SRCS
    src/sound_system.cpp
    INC_PUBLIC
    miniaudio
    opus
    INC_PRIVATE
    CORE::PRIMITIVES
)

add_subdirectory(test)
