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
    src/operation_coverage.cpp
    INC_PUBLIC
    INC_PRIVATE
    CORE::PRIMITIVES
    CORE::CONFIGURATION
    CORE::MODELS
    INFRASTRUCTURE::FILE_HANDLER
    INFRASTRUCTURE::DATABASE
    INFRASTRUCTURE::SPEECH_RECOGNITION
    INFRASTRUCTURE::EMBEDDINGS
    INFRASTRUCTURE::SIMILARITY
)

# add_subdirectory(mocks)
add_subdirectory(test)