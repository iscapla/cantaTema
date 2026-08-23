include(library_install_test)

create_test_exec(
    TARGET
        "t_similarity"
    SRCS
        t_similarity_main.cxx
        t_similarity_search.cxx
        t_word_sequence_aligner.cxx
        t_phonetic_matcher.cxx
        t_rubric_checklist_extractor.cxx
    INC_PRIVATE
        CORE::PRIMITIVES
        CORE::CONFIGURATION
        INFRASTRUCTURE::SIMILARITY
)

