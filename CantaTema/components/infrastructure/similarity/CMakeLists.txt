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
    INC_PUBLIC
    INC_PRIVATE
    CORE::PRIMITIVES
    CORE::CONFIGURATION
    INFRASTRUCTURE::FILE_HANDLER
)


add_subdirectory(test)
