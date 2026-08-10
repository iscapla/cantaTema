#include "embeddings/llama_embedding_engine.hpp"
#include "embeddings/llama_context_impl.hpp"
#include "configuration/configuration_system.hpp"
#include "primitives/utils_logger.hpp"

LlamaEmbeddingEngine::LlamaEmbeddingEngine(void)
    : m_context(std::make_unique<LlamaContextImpl>()) {
}

LlamaEmbeddingEngine::LlamaEmbeddingEngine(std::unique_ptr<LlamaContext> context)
    : m_context(std::move(context)) {
}

bool LlamaEmbeddingEngine::load_model(const std::filesystem::path& model_path) {
    if (!m_context) {
        logger->error("load_model called but context is null.");
        return false;
    }

    ConfigurationSystem& config = ConfigurationSystem::getInstance();
    int gpu_layers = config.get_embeddings_gpu_offload_layers();

    return m_context->load_model(model_path, gpu_layers);
}

std::vector<float> LlamaEmbeddingEngine::generate_embedding(const std::string& text, EmbeddingRole role) {
    if (!m_context) {
        logger->error("generate_embedding called but context is null.");
        return {};
    }

    std::string formatted_text = text;
    ConfigurationSystem& config = ConfigurationSystem::getInstance();
    if (config.get_embeddings_use_role_prefixes()) {
        if (role == EmbeddingRole::PASSAGE) {
            std::string prefix = config.get_embeddings_passage_prefix();
            if (formatted_text.rfind(prefix, 0) != 0) {
                formatted_text = prefix + formatted_text;
            }
        } else if (role == EmbeddingRole::QUERY) {
            std::string prefix = config.get_embeddings_query_prefix();
            if (formatted_text.rfind(prefix, 0) != 0) {
                formatted_text = prefix + formatted_text;
            }
        }
    }

    std::vector<int32_t> tokens;
    int n_tokens = m_context->tokenize(formatted_text, tokens);
    if (n_tokens < 0) {
        logger->error("Tokenization failed during generate_embedding.");
        return {};
    }

    return m_context->decode_and_get_embedding(tokens);
}

std::vector<std::vector<float>> LlamaEmbeddingEngine::generate_embeddings_batch(const std::vector<std::string>& texts, EmbeddingRole role) {
    std::vector<std::vector<float>> results;
    results.reserve(texts.size());
    for (const auto& text : texts) {
        results.push_back(generate_embedding(text, role));
    }
    return results;
}

size_t LlamaEmbeddingEngine::get_embedding_dimension(void) const {
    if (!m_context) {
        return 0;
    }
    return m_context->get_embedding_dimension();
}
