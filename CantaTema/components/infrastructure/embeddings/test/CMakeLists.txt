include(library_install_test)

create_test_exec(
    TARGET
        "t_embeddings"
    SRCS
        t_embeddings_main.cxx
        t_llama_embedding_engine.cxx
    INC_PRIVATE
        CORE::PRIMITIVES
        CORE::CONFIGURATION
        INFRASTRUCTURE::EMBEDDINGS
        llama
)
