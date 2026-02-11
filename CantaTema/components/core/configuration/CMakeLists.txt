include(library_install_infra)

create_infra_library(
    TARGET
    "configuration"
    SRCS
    src/i_configuration_base.cpp
    src/configuration_system.cpp
    INC_PUBLIC
    simpleini
    INC_PRIVATE
    CORE::PRIMITIVES
)

add_subdirectory(test)
