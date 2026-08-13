include(library_install_infra)

create_infra_library(
    TARGET
    "speech_recognition"
    SRCS
    src/whisper_speech_recognition.cpp
    src/whisper_engine_wrapper.cpp
    src/voice_quality_analyzer.cpp
    src/gpu_detector.cpp
    src/whisper_ctc_aligner.cpp
    src/ctc_alignment_manager.cpp
    INC_PUBLIC
    INFRASTRUCTURE::SOUND_SYSTEM
    whisper
    ggml
    SDL3::SDL3
    INC_PRIVATE
    CORE::PRIMITIVES
    CORE::CONFIGURATION
)


add_subdirectory(test)
