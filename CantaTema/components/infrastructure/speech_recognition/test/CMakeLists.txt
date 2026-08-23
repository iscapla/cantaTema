include(library_install_test)

create_test_exec(
    TARGET
        "t_speech_recognition"
    SRCS
        t_speech_recognition_main.cxx
        t_whisper_speech_recognition.cxx
        t_whisper_config_and_init.cxx
        t_whisper_audio_conversion.cxx
        t_whisper_transcription_engine.cxx
        t_voice_quality_analyzer.cxx
        t_gpu_detector.cxx
        t_whisper_quijote_integration.cxx
        t_whisper_coverage.cxx
        t_ctc_aligner.cxx
        t_diagnostic_radar_evaluator.cxx
        t_transcript_sentence_stitcher.cxx
    INC_PRIVATE
        CORE::PRIMITIVES
        CORE::CONFIGURATION
        INFRASTRUCTURE::SOUND_SYSTEM
        INFRASTRUCTURE::SPEECH_RECOGNITION
        whisper
        ggml
)

