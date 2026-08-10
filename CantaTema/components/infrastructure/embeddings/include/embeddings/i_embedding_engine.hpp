/**
 * @file i_embedding_engine.hpp
 * @brief Abstract interface for text embedding engines generating high-dimensional semantic vectors.
 */

#ifndef I_EMBEDDING_ENGINE_HPP
#define I_EMBEDDING_ENGINE_HPP

#include <string>
#include <vector>
#include <filesystem>
#include "primitives/definitions.hpp"

/**
 * @enum EmbeddingRole
 * @brief Role / context type of text for asymmetric embedding models (e.g. e5 models requiring passage: or query: prefixes).
 */
enum class EmbeddingRole {
    DEFAULT = 0,
    PASSAGE,
    QUERY
};

/**
 * @class IEmbeddingEngine
 * @brief Abstract interface for loading text embedding models and calculating vector embeddings for text chunks.
 */
class IEmbeddingEngine {
public:
    /**
     * @brief Virtual destructor for IEmbeddingEngine.
     */
    virtual ~IEmbeddingEngine() = default;

    /**
     * @brief Loads the GGUF embedding model from the specified file path.
     * 
     * @param model_path Path to the GGUF model file.
     * @return true if loading succeeded, false otherwise.
     */
    virtual bool load_model(const std::filesystem::path& model_path) = 0;

    /**
     * @brief Generates a vector embedding for a single string text.
     * 
     * @param text The input text to vectorize.
     * @param role Context role (DEFAULT, PASSAGE, QUERY) for optional prompt prefixing.
     * @return std::vector<float> Normalized embedding vector. Returns empty vector on error.
     */
    virtual std::vector<float> generate_embedding(const std::string& text, EmbeddingRole role = EmbeddingRole::DEFAULT) = 0;

    /**
     * @brief Generates vector embeddings for a batch of strings.
     * 
     * @param texts Input strings to vectorize.
     * @param role Context role (DEFAULT, PASSAGE, QUERY) for optional prompt prefixing.
     * @return std::vector<std::vector<float>> Normalized embedding vectors. Returns empty vector on error.
     */
    virtual std::vector<std::vector<float>> generate_embeddings_batch(const std::vector<std::string>& texts, EmbeddingRole role = EmbeddingRole::DEFAULT) = 0;

    /**
     * @brief Returns the dimension size of the embedding vectors produced by the loaded model.
     * 
     * @return size_t Dimension size (e.g. 1024 for e5-large).
     */
    virtual size_t get_embedding_dimension() const = 0;
};

#endif // I_EMBEDDING_ENGINE_HPP
