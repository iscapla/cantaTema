include(library_install_infra)

create_infra_library(
    TARGET
    "session"
    SRCS
    src/Session.cpp
    INC_PUBLIC
    BUSINESS::OPERATIONS
    CORE::PRIMITIVES
    CORE::CONFIGURATION
    INC_PRIVATE
)

# add_subdirectory(mocks)
# add_subdirectory(test)

