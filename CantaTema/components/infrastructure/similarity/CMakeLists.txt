include(library_install_infra)

create_infra_library(
    TARGET
    "similarity"
    SRCS
    src/faiss_similarity_search.cpp
    src/word_sequence_aligner.cpp
    src/double_metaphone_matcher.cpp
    src/soundex_matcher.cpp
    src/phonetic_matcher_manager.cpp
    src/rubric_checklist_extractor.cpp
    src/dictionary_paraphrase_matcher.cpp
    src/embedding_paraphrase_matcher.cpp
    src/hybrid_paraphrase_matcher.cpp
    src/paraphrase_matcher_manager.cpp
    INC_PUBLIC
    INC_PRIVATE
    CORE::PRIMITIVES
    CORE::CONFIGURATION
    INFRASTRUCTURE::FILE_HANDLER
    INFRASTRUCTURE::EMBEDDINGS
)


add_subdirectory(test)
