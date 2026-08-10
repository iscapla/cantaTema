/**
 * @file llama_embedding_engine.hpp
 * @brief Concrete implementation of IEmbeddingEngine using llama.cpp context in vector embedding mode.
 */

#ifndef LLAMA_EMBEDDING_ENGINE_HPP
#define LLAMA_EMBEDDING_ENGINE_HPP

#include "embeddings/i_embedding_engine.hpp"
#include "embeddings/llama_context.hpp"
#include <memory>

/**
 * @class LlamaEmbeddingEngine
 * @brief Vector embedding engine implementation executing GGUF embedding models via llama.cpp.
 */
class LlamaEmbeddingEngine : public IEmbeddingEngine {
public:
    /**
     * @brief Default constructor initializing default LlamaContext.
     */
    LlamaEmbeddingEngine(void);
    
    /**
     * @brief Constructs LlamaEmbeddingEngine with an injected LlamaContext (useful for unit testing).
     * @param context Unique pointer to custom or mock LlamaContext instance.
     */
    explicit LlamaEmbeddingEngine(std::unique_ptr<LlamaContext> context);
    
    /**
     * @brief Destructor for LlamaEmbeddingEngine.
     */
    ~LlamaEmbeddingEngine(void) override = default;

    /**
     * @brief Loads a GGUF vector embedding model from disk.
     * @param model_path Path to the .gguf model file.
     * @return true if model was loaded successfully, false otherwise.
     */
    bool load_model(const std::filesystem::path& model_path) override;

    /**
     * @brief Generates a vector embedding for a single text string.
     * @param text Input text chunk.
     * @param role Context role (DEFAULT, PASSAGE, QUERY) for optional prompt prefixing.
     * @return std::vector<float> Normalized vector embedding output.
     */
    std::vector<float> generate_embedding(const std::string& text, EmbeddingRole role = EmbeddingRole::DEFAULT) override;

    /**
     * @brief Generates vector embeddings for a batch array of text strings.
     * @param texts Array of input text chunks.
     * @param role Context role (DEFAULT, PASSAGE, QUERY) for optional prompt prefixing.
     * @return std::vector<std::vector<float>> Vector array of normalized embeddings.
     */
    std::vector<std::vector<float>> generate_embeddings_batch(const std::vector<std::string>& texts, EmbeddingRole role = EmbeddingRole::DEFAULT) override;

    /**
     * @brief Returns the vector dimension produced by the loaded embedding model.
     * @return size_t Dimension size (e.g. 1024).
     */
    size_t get_embedding_dimension() const override;

private:
    std::unique_ptr<LlamaContext> m_context;
};

#endif // LLAMA_EMBEDDING_ENGINE_HPP
