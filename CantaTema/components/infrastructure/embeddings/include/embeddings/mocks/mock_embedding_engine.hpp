#ifndef MOCK_EMBEDDING_ENGINE_HPP
#define MOCK_EMBEDDING_ENGINE_HPP

#include <gmock/gmock.h>
#include "embeddings/i_embedding_engine.hpp"

class MockEmbeddingEngine : public IEmbeddingEngine {
public:
    MOCK_METHOD(bool, load_model, (const std::filesystem::path& model_path), (override));
    MOCK_METHOD(std::vector<float>, generate_embedding, (const std::string& text), (override));
    MOCK_METHOD(std::vector<std::vector<float>>, generate_embeddings_batch, (const std::vector<std::string>& texts), (override));
    MOCK_METHOD(size_t, get_embedding_dimension, (), (const, override));
};

#endif // MOCK_EMBEDDING_ENGINE_HPP
