#ifndef LLAMA_CONTEXT_IMPL_HPP
#define LLAMA_CONTEXT_IMPL_HPP

#include "embeddings/llama_context.hpp"
#include <llama.h>
#include <functional>

/**
 * @brief Concrete implementation of LlamaContext wrapping the llama.cpp C API.
 *
 * All calls to llama.cpp C functions are routed through static
 * `std::function` dispatch entries. Tests can replace individual
 * entries via setup_mocks() / reset_mocks() to exercise error paths
 * without requiring a real GGUF model file on disk.
 */
class LlamaContextImpl : public LlamaContext {
public:
    LlamaContextImpl(void);
    ~LlamaContextImpl(void) override;

    // ---------------------------------------------------------------------------
    // Dispatch table — static members pointing to llama.cpp C-API functions.
    // Tests override individual entries to control behavior without a real model.
    // ---------------------------------------------------------------------------
    static std::function<void(void)>                                                                    fn_llama_backend_init;
    static std::function<llama_model_params(void)>                                                     fn_llama_model_default_params;
    static std::function<llama_model*(const char*, llama_model_params)>                                fn_llama_model_load_from_file;
    static std::function<llama_context_params(void)>                                                   fn_llama_context_default_params;
    static std::function<llama_context*(llama_model*, llama_context_params)>                           fn_llama_init_from_model;
    static std::function<const llama_vocab*(const llama_model*)>                                       fn_llama_model_get_vocab;
    static std::function<int(const llama_vocab*, const char*, int, llama_token*, int, bool, bool)>     fn_llama_tokenize;
    static std::function<llama_memory_t(const llama_context*)>                                        fn_llama_get_memory;
    static std::function<void(llama_memory_t, bool)>                                                   fn_llama_memory_clear;
    static std::function<int(llama_context*, llama_batch)>                                             fn_llama_decode;
    static std::function<float*(llama_context*, int)>                                                  fn_llama_get_embeddings_ith;
    static std::function<float*(llama_context*, int32_t)>                                              fn_llama_get_embeddings_seq;
    static std::function<float*(llama_context*)>                                                       fn_llama_get_embeddings;
    static std::function<int(const llama_model*)>                                                 fn_llama_model_n_embd;
    static std::function<void(llama_context*)>                                                    fn_llama_free;
    static std::function<void(llama_model*)>                                                      fn_llama_model_free;

    /// @brief Replace all dispatch entries with lightweight stubs suitable for unit testing.
    static void setup_mocks(void);
    /// @brief Restore all dispatch entries to the real llama.cpp C-API functions.
    static void reset_mocks(void);

    bool load_model(const std::filesystem::path& model_path, int gpu_offload_layers) override;
    int  tokenize(const std::string& text, std::vector<int32_t>& out_tokens) override;
    std::vector<float> decode_and_get_embedding(const std::vector<int32_t>& tokens) override;
    size_t get_embedding_dimension() const override;

private:
    llama_model*   m_model;
    llama_context* m_ctx;

    void free_resources(void);
};

#endif // LLAMA_CONTEXT_IMPL_HPP
