include(library_install_test)

create_test_exec(
    TARGET
        "t_similarity"
    SRCS
        t_similarity_main.cxx
        t_similarity_search.cxx
        t_word_sequence_aligner.cxx
    INC_PRIVATE
        CORE::PRIMITIVES
        INFRASTRUCTURE::SIMILARITY
)
