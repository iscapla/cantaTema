include(library_install_infra)

create_infra_library(
    TARGET
    "models"
    SRCS
    src/manager_whisper.cpp
    INC_PUBLIC
    INC_PRIVATE
    libcurl
    CORE::LOGGER
)

add_subdirectory(test)
