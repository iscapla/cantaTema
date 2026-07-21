include(library_install_infra)

create_infra_library(
    TARGET
    "embeddings"
    SRCS
    src/llama_embedding_engine.cpp
    src/llama_context_impl.cpp
    INC_PUBLIC
    INC_PRIVATE
    llama
    CORE::PRIMITIVES
    CORE::CONFIGURATION
)

add_subdirectory(test)
