include(library_install_infra)

create_infra_library(
    TARGET
    "configuration"
    SRCS
    src/IConfigurationBase.cpp
    src/Configuration_System.cpp
    INC_PUBLIC
    simpleini
    INC_PRIVATE
    CORE::PRIMITIVES
)
