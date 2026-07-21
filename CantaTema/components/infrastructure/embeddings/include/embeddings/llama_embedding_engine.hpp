#ifndef LLAMA_EMBEDDING_ENGINE_HPP
#define LLAMA_EMBEDDING_ENGINE_HPP

#include "embeddings/i_embedding_engine.hpp"
#include "embeddings/llama_context.hpp"
#include <memory>

class LlamaEmbeddingEngine : public IEmbeddingEngine {
public:
    // Default constructor uses LlamaContextImpl
    LlamaEmbeddingEngine(void);
    
    // Inject mock context in tests
    explicit LlamaEmbeddingEngine(std::unique_ptr<LlamaContext> context);
    
    ~LlamaEmbeddingEngine(void) override = default;

    bool load_model(const std::filesystem::path& model_path) override;
    std::vector<float> generate_embedding(const std::string& text) override;
    std::vector<std::vector<float>> generate_embeddings_batch(const std::vector<std::string>& texts) override;
    size_t get_embedding_dimension() const override;

private:
    std::unique_ptr<LlamaContext> m_context;
};

#endif // LLAMA_EMBEDDING_ENGINE_HPP
