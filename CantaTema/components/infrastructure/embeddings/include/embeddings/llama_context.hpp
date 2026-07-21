#ifndef LLAMA_CONTEXT_HPP
#define LLAMA_CONTEXT_HPP

#include <string>
#include <vector>
#include <filesystem>
#include <cstdint>

class LlamaContext {
public:
    virtual ~LlamaContext() = default;

    /**
     * @brief Loads the GGUF model and creates context.
     * 
     * @param model_path Path to the GGUF model.
     * @param gpu_offload_layers Number of layers to offload to GPU.
     * @return true if loading succeeded, false otherwise.
     */
    virtual bool load_model(const std::filesystem::path& model_path, int gpu_offload_layers) = 0;

    /**
     * @brief Tokenizes the given text.
     * 
     * @param text Input text.
     * @param out_tokens Output vector of tokens.
     * @return int Number of tokens, or negative on error.
     */
    virtual int tokenize(const std::string& text, std::vector<int32_t>& out_tokens) = 0;

    /**
     * @brief Decodes the tokens and returns the normalized embedding vector.
     * 
     * @param tokens Input tokens.
     * @return std::vector<float> Normalized embedding vector. Empty on error.
     */
    virtual std::vector<float> decode_and_get_embedding(const std::vector<int32_t>& tokens) = 0;

    /**
     * @brief Returns the embedding dimension of the loaded model.
     * 
     * @return size_t Dimension size.
     */
    virtual size_t get_embedding_dimension() const = 0;
};

#endif // LLAMA_CONTEXT_HPP
