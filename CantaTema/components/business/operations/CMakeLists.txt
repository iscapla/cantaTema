include(library_install_infra)

create_infra_library(
    TARGET
    "operations"
    SRCS
    src/operation_user.cpp
    src/operation_category.cpp
    src/operation_subject.cpp
    src/operation_user_metrics.cpp
    src/operation_practice_event.cpp
    INC_PUBLIC
    INC_PRIVATE
    CORE::PRIMITIVES
    CORE::CONFIGURATION
    INFRASTRUCTURE::FILE_HANDLER
    INFRASTRUCTURE::DATABASE
)

# add_subdirectory(mocks)
# add_subdirectory(test)