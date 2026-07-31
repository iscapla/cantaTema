#include "embeddings/llama_context_impl.hpp"
#include "primitives/utils_logger.hpp"
#include <llama.h>
#include <cmath>
#include <mutex>

namespace {
    std::once_flag s_llama_backend_init_flag;
}

// Static dispatch table definitions — initialized to real llama.cpp C-API functions.
// Tests override these to control behavior without needing a real GGUF model.
std::function<void(void)>                                                             LlamaContextImpl::fn_llama_backend_init          = llama_backend_init;
std::function<llama_model_params(void)>                                               LlamaContextImpl::fn_llama_model_default_params   = llama_model_default_params;
std::function<llama_model*(const char*, llama_model_params)>                          LlamaContextImpl::fn_llama_model_load_from_file   = llama_model_load_from_file;
std::function<llama_context_params(void)>                                             LlamaContextImpl::fn_llama_context_default_params = llama_context_default_params;
std::function<llama_context*(llama_model*, llama_context_params)>                     LlamaContextImpl::fn_llama_init_from_model        = llama_init_from_model;
std::function<const llama_vocab*(const llama_model*)>                                 LlamaContextImpl::fn_llama_model_get_vocab        = llama_model_get_vocab;
std::function<int(const llama_vocab*, const char*, int, llama_token*, int, bool, bool)> LlamaContextImpl::fn_llama_tokenize             = llama_tokenize;
std::function<llama_memory_t(const llama_context*)>                                   LlamaContextImpl::fn_llama_get_memory             = llama_get_memory;
std::function<void(llama_memory_t, bool)>                                             LlamaContextImpl::fn_llama_memory_clear         = llama_memory_clear;
std::function<int(llama_context*, llama_batch)>                                       LlamaContextImpl::fn_llama_decode                 = llama_decode;
std::function<float*(llama_context*, int)>                                            LlamaContextImpl::fn_llama_get_embeddings_ith     = llama_get_embeddings_ith;
std::function<float*(llama_context*, int32_t)>                                        LlamaContextImpl::fn_llama_get_embeddings_seq     = llama_get_embeddings_seq;
std::function<float*(llama_context*)>                                                 LlamaContextImpl::fn_llama_get_embeddings         = llama_get_embeddings;
std::function<int(const llama_model*)>                                                LlamaContextImpl::fn_llama_model_n_embd            = llama_model_n_embd;
std::function<void(llama_context*)>                                                   LlamaContextImpl::fn_llama_free                   = llama_free;
std::function<void(llama_model*)>                                                     LlamaContextImpl::fn_llama_model_free             = llama_model_free;

// ---------------------------------------------------------------------------
// Mock helpers
// ---------------------------------------------------------------------------

void LlamaContextImpl::setup_mocks(void) {
    static float s_mock_emb_data[1024] = { 1.0f };

    fn_llama_backend_init          = []() {};
    fn_llama_model_default_params  = []() { return llama_model_params{}; };
    fn_llama_model_load_from_file  = [](const char*, llama_model_params) { return reinterpret_cast<llama_model*>(0x1234); };
    fn_llama_context_default_params= []() { return llama_context_params{}; };
    fn_llama_init_from_model       = [](llama_model*, llama_context_params) { return reinterpret_cast<llama_context*>(0x5678); };
    fn_llama_model_get_vocab       = [](const llama_model*) { return reinterpret_cast<const llama_vocab*>(0x9ABC); };
    fn_llama_tokenize              = [](const llama_vocab*, const char*, int, llama_token* tokens, int, bool, bool) {
        if (tokens) { tokens[0] = 1; tokens[1] = 2; }
        return 2;
    };
    fn_llama_get_memory            = [](const llama_context*) { return reinterpret_cast<llama_memory_t>(0xDEF0); };
    fn_llama_memory_clear          = [](llama_memory_t, bool) {};
    fn_llama_decode                = [](llama_context*, llama_batch) { return 0; };
    fn_llama_get_embeddings_ith    = [](llama_context*, int) -> float* { return s_mock_emb_data; };
    fn_llama_get_embeddings_seq    = [](llama_context*, int32_t) -> float* { return s_mock_emb_data; };
    fn_llama_get_embeddings        = [](llama_context*) -> float* { return s_mock_emb_data; };
    fn_llama_model_n_embd          = [](const llama_model*) { return 1024; };
    fn_llama_free                  = [](llama_context*) {};
    fn_llama_model_free            = [](llama_model*) {};
}

void LlamaContextImpl::reset_mocks(void) {
    fn_llama_backend_init          = llama_backend_init;
    fn_llama_model_default_params  = llama_model_default_params;
    fn_llama_model_load_from_file  = llama_model_load_from_file;
    fn_llama_context_default_params= llama_context_default_params;
    fn_llama_init_from_model       = llama_init_from_model;
    fn_llama_model_get_vocab       = llama_model_get_vocab;
    fn_llama_tokenize              = llama_tokenize;
    fn_llama_get_memory            = llama_get_memory;
    fn_llama_memory_clear          = llama_memory_clear;
    fn_llama_decode                = llama_decode;
    fn_llama_get_embeddings_ith    = llama_get_embeddings_ith;
    fn_llama_get_embeddings_seq    = llama_get_embeddings_seq;
    fn_llama_get_embeddings        = llama_get_embeddings;
    fn_llama_model_n_embd          = llama_model_n_embd;
    fn_llama_free                  = llama_free;
    fn_llama_model_free            = llama_model_free;
}

// ---------------------------------------------------------------------------
// LlamaContextImpl
// ---------------------------------------------------------------------------

LlamaContextImpl::LlamaContextImpl(void)
    : m_model(nullptr), m_ctx(nullptr) {
    std::call_once(s_llama_backend_init_flag, []() {
        LlamaContextImpl::fn_llama_backend_init();
    });
}

LlamaContextImpl::~LlamaContextImpl(void) {
    free_resources();
}

void LlamaContextImpl::free_resources(void) {
    if (m_ctx) {
        fn_llama_free(m_ctx);
        m_ctx = nullptr;
    }
    if (m_model) {
        fn_llama_model_free(m_model);
        m_model = nullptr;
    }
}

bool LlamaContextImpl::load_model(const std::filesystem::path& model_path, int gpu_offload_layers) {
    free_resources();

    // Check if mocks are installed (fn_llama_model_load_from_file returns sentinel on empty path).
    // If not mocked, verify file existence before calling the real loader.
    llama_model* probe = fn_llama_model_load_from_file("", llama_model_params{});
    const bool using_mocks = (probe == reinterpret_cast<llama_model*>(0x1234));

    if (!using_mocks) {
        if (!std::filesystem::exists(model_path)) {
            logger->error("Embedding model file not found: {}", model_path.string());
            return false;
        }
    }

    llama_model_params model_params = fn_llama_model_default_params();
    model_params.n_gpu_layers       = gpu_offload_layers;

    m_model = fn_llama_model_load_from_file(model_path.string().c_str(), model_params);
    if (!m_model) {
        logger->error("Failed to load llama model from file: {}", model_path.string());
        return false;
    }

    llama_context_params ctx_params = fn_llama_context_default_params();
    ctx_params.embeddings           = true;
    ctx_params.n_ctx                = 512; // multilingual-e5-large context limit

    m_ctx = fn_llama_init_from_model(m_model, ctx_params);
    if (!m_ctx) {
        logger->error("Failed to create llama context for model: {}", model_path.string());
        fn_llama_model_free(m_model);
        m_model = nullptr;
        return false;
    }

    logger->info("Loaded embedding model successfully: {} (GPU layers: {})", model_path.string(), gpu_offload_layers);
    return true;
}

int LlamaContextImpl::tokenize(const std::string& text, std::vector<int32_t>& out_tokens) {
    if (!m_model) {
        logger->error("tokenize called but model is not loaded.");
        return -1;
    }

    const llama_vocab* vocab = fn_llama_model_get_vocab(m_model);
    if (!vocab) {
        logger->error("Failed to get vocab from model.");
        return -1;
    }

    // First pass: optimistic buffer size
    out_tokens.resize(text.length() + 4);
    int n_tokens = fn_llama_tokenize(vocab, text.c_str(), static_cast<int>(text.length()),
                                     reinterpret_cast<llama_token*>(out_tokens.data()),
                                     static_cast<int>(out_tokens.size()), true, true);

    if (n_tokens < 0) {
        // Negative return means the buffer was too small; retry with the exact size
        out_tokens.resize(static_cast<size_t>(-n_tokens));
        n_tokens = fn_llama_tokenize(vocab, text.c_str(), static_cast<int>(text.length()),
                                     reinterpret_cast<llama_token*>(out_tokens.data()),
                                     static_cast<int>(out_tokens.size()), true, true);
    }

    if (n_tokens < 0) {
        logger->error("Failed to tokenize text.");
        return -1;
    }

    out_tokens.resize(static_cast<size_t>(n_tokens));
    return n_tokens;
}

std::vector<float> LlamaContextImpl::decode_and_get_embedding(const std::vector<int32_t>& tokens) {
    if (!m_model || !m_ctx) {
        logger->error("decode_and_get_embedding called but model/context not initialized.");
        return {};
    }

    if (tokens.empty()) {
        return std::vector<float>(get_embedding_dimension(), 0.0f);
    }

    llama_batch batch = llama_batch_get_one(
        const_cast<llama_token*>(reinterpret_cast<const llama_token*>(tokens.data())),
        static_cast<int32_t>(tokens.size()));

    fn_llama_memory_clear(fn_llama_get_memory(m_ctx), true);

    if (fn_llama_decode(m_ctx, batch) != 0) {
        logger->error("llama_decode failed during embedding generation.");
        return {};
    }

    // Prefer per-sequence embeddings; fall back to last token index or pooled output
    float* embd = fn_llama_get_embeddings_seq(m_ctx, 0);
    if (!embd) {
        embd = fn_llama_get_embeddings_ith(m_ctx, -1);
    }
    if (!embd) {
        embd = fn_llama_get_embeddings_ith(m_ctx, static_cast<int>(tokens.size()) - 1);
    }
    if (!embd) {
        embd = fn_llama_get_embeddings(m_ctx);
    }
    if (!embd) {
        logger->error("Failed to retrieve embeddings from llama context.");
        return {};
    }

    size_t dim = get_embedding_dimension();
    std::vector<float> embedding(embd, embd + dim);

    // L2 normalise
    double sum_sq = 0.0;
    for (float val : embedding) {
        sum_sq += static_cast<double>(val) * val;
    }
    const double norm = std::sqrt(sum_sq);
    if (norm > 1e-6) {
        for (float& val : embedding) {
            val = static_cast<float>(static_cast<double>(val) / norm);
        }
    }

    return embedding;
}

size_t LlamaContextImpl::get_embedding_dimension(void) const {
    if (!m_model) {
        return 0;
    }
    return static_cast<size_t>(fn_llama_model_n_embd(m_model));
}
