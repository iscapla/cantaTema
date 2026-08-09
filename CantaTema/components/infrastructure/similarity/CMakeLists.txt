include(library_install_infra)

create_infra_library(
    TARGET
    "similarity"
    SRCS
    src/faiss_similarity_search.cpp
    INC_PUBLIC
    INC_PRIVATE
    CORE::PRIMITIVES
    INFRASTRUCTURE::FILE_HANDLER
)

add_subdirectory(test)
