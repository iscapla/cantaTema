#ifndef MOCK_EMBEDDING_ENGINE_HPP
#define MOCK_EMBEDDING_ENGINE_HPP

#include <gmock/gmock.h>
#include "embeddings/i_embedding_engine.hpp"

class MockEmbeddingEngine : public IEmbeddingEngine {
public:
    using FloatVector = std::vector<float>;
    using FloatVectorBatch = std::vector<std::vector<float>>;

    MOCK_METHOD(bool, load_model, (const std::filesystem::path&), (override));
    MOCK_METHOD(FloatVector, generate_embedding, (const std::string&, EmbeddingRole), (override));
    MOCK_METHOD(FloatVectorBatch, generate_embeddings_batch, (const std::vector<std::string>&, EmbeddingRole), (override));
    MOCK_METHOD(size_t, get_embedding_dimension, (), (const, override));
};

#endif // MOCK_EMBEDDING_ENGINE_HPP
