#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <fstream>
#include <cmath>
#include <llama.h>
#include "embeddings/llama_embedding_engine.hpp"
#include "embeddings/llama_context.hpp"
#include "embeddings/llama_context_impl.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::SetArgReferee;

class MockLlamaContext : public LlamaContext {
public:
    MOCK_METHOD(bool, load_model, (const std::filesystem::path&, int), (override));
    MOCK_METHOD(int, tokenize, (const std::string&, std::vector<int32_t>&), (override));
    MOCK_METHOD(std::vector<float>, decode_and_get_embedding, (const std::vector<int32_t>&), (override));
    MOCK_METHOD(size_t, get_embedding_dimension, (), (const, override));
};

class LlamaEmbeddingEngineTest : public ::testing::Test {
protected:
    std::unique_ptr<MockLlamaContext> mock_ctx;
    MockLlamaContext* mock_ctx_ptr;
    std::unique_ptr<LlamaEmbeddingEngine> engine;

    void SetUp() override {
        mock_ctx = std::make_unique<MockLlamaContext>();
        mock_ctx_ptr = mock_ctx.get();
        engine = std::make_unique<LlamaEmbeddingEngine>(std::move(mock_ctx));
    }
};

TEST_F(LlamaEmbeddingEngineTest, LoadModelSuccess) {
    EXPECT_CALL(*mock_ctx_ptr, load_model(std::filesystem::path("test_model.gguf"), _))
        .WillOnce(Return(true));

    EXPECT_TRUE(engine->load_model("test_model.gguf"));
}

TEST_F(LlamaEmbeddingEngineTest, LoadModelFailure) {
    EXPECT_CALL(*mock_ctx_ptr, load_model(std::filesystem::path("invalid.gguf"), _))
        .WillOnce(Return(false));

    EXPECT_FALSE(engine->load_model("invalid.gguf"));
}

TEST_F(LlamaEmbeddingEngineTest, GenerateEmbeddingSuccess) {
    std::string text = "Hola Mundo";
    std::vector<int32_t> dummy_tokens = {1, 2, 3};
    std::vector<float> expected_emb = {0.1f, 0.2f, 0.3f};

    EXPECT_CALL(*mock_ctx_ptr, tokenize(text, _))
        .WillOnce(::testing::DoAll(SetArgReferee<1>(dummy_tokens), Return(3)));

    EXPECT_CALL(*mock_ctx_ptr, decode_and_get_embedding(dummy_tokens))
        .WillOnce(Return(expected_emb));

    auto emb = engine->generate_embedding(text);
    EXPECT_EQ(emb, expected_emb);
}

TEST_F(LlamaEmbeddingEngineTest, GenerateEmbeddingTokenizeFailure) {
    std::string text = "Error text";

    EXPECT_CALL(*mock_ctx_ptr, tokenize(text, _))
        .WillOnce(Return(-1));

    auto emb = engine->generate_embedding(text);
    EXPECT_TRUE(emb.empty());
}

TEST_F(LlamaEmbeddingEngineTest, GenerateEmbeddingsBatch) {
    std::vector<std::string> texts = {"Frase uno", "Frase dos"};
    std::vector<int32_t> tokens1 = {10};
    std::vector<int32_t> tokens2 = {20};
    std::vector<float> emb1 = {0.5f};
    std::vector<float> emb2 = {0.9f};

    EXPECT_CALL(*mock_ctx_ptr, tokenize("Frase uno", _))
        .WillOnce(::testing::DoAll(SetArgReferee<1>(tokens1), Return(1)));
    EXPECT_CALL(*mock_ctx_ptr, decode_and_get_embedding(tokens1))
        .WillOnce(Return(emb1));

    EXPECT_CALL(*mock_ctx_ptr, tokenize("Frase dos", _))
        .WillOnce(::testing::DoAll(SetArgReferee<1>(tokens2), Return(1)));
    EXPECT_CALL(*mock_ctx_ptr, decode_and_get_embedding(tokens2))
        .WillOnce(Return(emb2));

    auto batch_embs = engine->generate_embeddings_batch(texts);
    ASSERT_EQ(batch_embs.size(), 2);
    EXPECT_EQ(batch_embs[0], emb1);
    EXPECT_EQ(batch_embs[1], emb2);
}

TEST_F(LlamaEmbeddingEngineTest, GetEmbeddingDimension) {
    EXPECT_CALL(*mock_ctx_ptr, get_embedding_dimension())
        .WillOnce(Return(1024));

    EXPECT_EQ(engine->get_embedding_dimension(), 1024);
}

TEST_F(LlamaEmbeddingEngineTest, NullContextHandling) {
    LlamaEmbeddingEngine null_engine(nullptr);

    EXPECT_FALSE(null_engine.load_model("test.gguf"));
    EXPECT_TRUE(null_engine.generate_embedding("text").empty());
    EXPECT_EQ(null_engine.get_embedding_dimension(), 0);
}

// Test LlamaContextImpl class directly to cover file checking logic
TEST(LlamaContextImplTest, LoadNonExistentModel) {
    LlamaContextImpl ctx;
    EXPECT_FALSE(ctx.load_model("non_existent_file_path_123.gguf", 0));
    EXPECT_EQ(ctx.get_embedding_dimension(), 0);
    
    std::vector<int32_t> tokens;
    EXPECT_EQ(ctx.tokenize("test", tokens), -1);
    
    EXPECT_TRUE(ctx.decode_and_get_embedding(tokens).empty());
}

TEST(LlamaContextImplTest, LoadEmptyModelFile) {
    // Write an empty file to test model loading failure path
    std::filesystem::path empty_file = "empty_model.gguf";
    std::ofstream ofs(empty_file);
    ofs.close();

    LlamaContextImpl ctx;
    // Should fail because it is not a valid GGUF file or fails check
    EXPECT_FALSE(ctx.load_model(empty_file, 0));

    std::filesystem::remove(empty_file);
}

TEST(LlamaContextImplTest, DefaultEngineConstructor) {
    // Make sure we can construct production engine
    LlamaEmbeddingEngine prod_engine;
    EXPECT_FALSE(prod_engine.load_model("non_existent_file_path_123.gguf"));
}

TEST(LlamaContextImplTest, LlamaContextImplExecutionSuccessPaths) {
    LlamaContextImpl::setup_mocks();
    {
        LlamaContextImpl ctx;
        EXPECT_TRUE(ctx.load_model("any_path_dummy.gguf", 0));
        EXPECT_EQ(ctx.get_embedding_dimension(), 1024);

        std::vector<int32_t> tokens;
        EXPECT_EQ(ctx.tokenize("test", tokens), 2);
        EXPECT_EQ(tokens.size(), 2u);

        auto emb = ctx.decode_and_get_embedding(tokens);
        EXPECT_EQ(emb.size(), 1024u);
        EXPECT_FLOAT_EQ(emb[0], 1.0f);

        // Empty tokens case coverage
        std::vector<int32_t> empty_tokens;
        auto empty_emb = ctx.decode_and_get_embedding(empty_tokens);
        EXPECT_EQ(empty_emb.size(), 1024u);
    }
    LlamaContextImpl::reset_mocks();
}

TEST(LlamaContextImplTest, ContextCreationFailure) {
    // Model loads OK but context creation returns nullptr → load_model should fail
    LlamaContextImpl::setup_mocks();
    LlamaContextImpl::fn_llama_new_context_with_model = [](llama_model*, llama_context_params) -> llama_context* {
        return nullptr;
    };
    {
        LlamaContextImpl ctx;
        EXPECT_FALSE(ctx.load_model("dummy.gguf", 0));
        // After failure, model and ctx should be cleaned up
        EXPECT_EQ(ctx.get_embedding_dimension(), 0u);
    }
    LlamaContextImpl::reset_mocks();
}

TEST(LlamaContextImplTest, TokenizeRetryOnSmallBuffer) {
    // First call returns -4 (needs 4 tokens); second call succeeds
    LlamaContextImpl::setup_mocks();
    int call_count = 0;
    LlamaContextImpl::fn_llama_tokenize = [&call_count](const llama_model*, const char*, int,
                                                         llama_token* tokens, int, bool, bool) -> int {
        ++call_count;
        if (call_count == 1) return -4;           // buffer too small
        if (tokens) { tokens[0] = 10; tokens[1] = 20; tokens[2] = 30; tokens[3] = 40; }
        return 4;
    };
    {
        LlamaContextImpl ctx;
        EXPECT_TRUE(ctx.load_model("dummy.gguf", 0));
        std::vector<int32_t> out;
        EXPECT_EQ(ctx.tokenize("text", out), 4);
        EXPECT_EQ(out.size(), 4u);
    }
    LlamaContextImpl::reset_mocks();
}

TEST(LlamaContextImplTest, TokenizeAlwaysFails) {
    // Both tokenize calls return negative → should return -1
    LlamaContextImpl::setup_mocks();
    LlamaContextImpl::fn_llama_tokenize = [](const llama_model*, const char*, int,
                                              llama_token*, int, bool, bool) -> int {
        return -1;
    };
    {
        LlamaContextImpl ctx;
        EXPECT_TRUE(ctx.load_model("dummy.gguf", 0));
        std::vector<int32_t> out;
        EXPECT_EQ(ctx.tokenize("text", out), -1);
    }
    LlamaContextImpl::reset_mocks();
}

TEST(LlamaContextImplTest, DecodeFailure) {
    // llama_decode returns non-zero → decode_and_get_embedding should return {}
    LlamaContextImpl::setup_mocks();
    LlamaContextImpl::fn_llama_decode = [](llama_context*, llama_batch) -> int { return -1; };
    {
        LlamaContextImpl ctx;
        EXPECT_TRUE(ctx.load_model("dummy.gguf", 0));
        std::vector<int32_t> tokens = {1, 2};
        auto emb = ctx.decode_and_get_embedding(tokens);
        EXPECT_TRUE(emb.empty());
    }
    LlamaContextImpl::reset_mocks();
}

TEST(LlamaContextImplTest, FallbackToGetEmbeddings) {
    // get_embeddings_ith returns nullptr; get_embeddings returns valid data.
    // s_fallback[0] = 0.5f, rest = 0.0f → L2 norm = 0.5f → normalised emb[0] = 1.0f.
    static float s_fallback[1024] = { 0.5f };
    LlamaContextImpl::setup_mocks();
    LlamaContextImpl::fn_llama_get_embeddings_ith = [](llama_context*, int) -> float* { return nullptr; };
    LlamaContextImpl::fn_llama_get_embeddings     = [](llama_context*)       -> float* { return s_fallback; };
    {
        LlamaContextImpl ctx;
        EXPECT_TRUE(ctx.load_model("dummy.gguf", 0));
        std::vector<int32_t> tokens = {1, 2};
        auto emb = ctx.decode_and_get_embedding(tokens);
        EXPECT_EQ(emb.size(), 1024u);
        // Only element [0] is non-zero (0.5f); norm = 0.5f → normalised value = 1.0f
        EXPECT_FLOAT_EQ(emb[0], 1.0f);
        // All other elements should be 0.0f
        for (size_t i = 1; i < emb.size(); ++i) {
            EXPECT_FLOAT_EQ(emb[i], 0.0f);
        }
    }
    LlamaContextImpl::reset_mocks();
}

TEST(LlamaContextImplTest, NullEmbeddingPointers) {
    // Both get_embeddings_ith and get_embeddings return nullptr → should return {}
    LlamaContextImpl::setup_mocks();
    LlamaContextImpl::fn_llama_get_embeddings_ith = [](llama_context*, int) -> float* { return nullptr; };
    LlamaContextImpl::fn_llama_get_embeddings     = [](llama_context*)       -> float* { return nullptr; };
    {
        LlamaContextImpl ctx;
        EXPECT_TRUE(ctx.load_model("dummy.gguf", 0));
        std::vector<int32_t> tokens = {1, 2};
        auto emb = ctx.decode_and_get_embedding(tokens);
        EXPECT_TRUE(emb.empty());
    }
    LlamaContextImpl::reset_mocks();
}

