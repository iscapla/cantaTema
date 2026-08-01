include(library_install_infra)

create_infra_library(
    TARGET
    "reports"
    SRCS
    src/whisper_accuracy_visualizer.cpp
    src/text_comparison_visualizer.cpp
    INC_PUBLIC
    CORE::PRIMITIVES
    INFRASTRUCTURE::SPEECH_RECOGNITION
    INC_PRIVATE
    CORE::CONFIGURATION
)

add_subdirectory(test)
