#ifndef __MANAGER_MODELS_HPP__
#define __MANAGER_MODELS_HPP__

#include "primitives/definitions.hpp"
#include <string>
#include <vector>
#include <functional>
#include <cstdio>
#include <filesystem>

enum class ModelType {
    Whisper,
    Llama
};

struct DownloadProgress {
    std::string file_name;
    size_t total_bytes;
    size_t downloaded_bytes;
};

using DownloadProgressCallback = std::function<void(const DownloadProgress&)>;

class ManagerModels {
public:
    struct ModelInfo {
        ModelType type;
        std::string name;
        std::string path;
        bool available_local = false;
        bool available_network = false;
    };

    /**
     * @brief Construct a new Manager Models object
     */
    ManagerModels();

    /**
     * @brief Destroy the Manager Models object
     */
    ~ManagerModels();

    /**
     * @brief Get all available models (Whisper and Llama)
     * 
     * @param check_network If true, checks the network for availability
     * @param models Output vector where the model info will be stored
     * @return rst_code_e RST_OK if successful
     */
    rst_code_e get_available_models(bool check_network, std::vector<ModelInfo>& models) const;

    /**
     * @brief Get available Whisper models
     */
    rst_code_e get_whisper_models(bool check_network, std::vector<ModelInfo>& models) const;

    /**
     * @brief Get available Llama embedding models
     */
    rst_code_e get_llama_models(bool check_network, std::vector<ModelInfo>& models) const;

    /**
     * @brief Check if a Whisper model is available locally
     */
    rst_code_e local_is_whisper_model_available(const std::string& model_name) const;

    /**
     * @brief Check if a Llama model is available locally
     */
    rst_code_e local_is_llama_model_available(const std::string& model_name) const;

    /**
     * @brief Remove a Whisper model from local disk
     */
    rst_code_e local_remove_whisper_model(const std::string& model_name) const;

    /**
     * @brief Remove a Llama model from local disk
     */
    rst_code_e local_remove_llama_model(const std::string& model_name) const;

    /**
     * @brief Download a model from Hugging Face
     * 
     * @param type Model type (Whisper or Llama)
     * @param model_name Name of the model
     * @param callback Progress callback
     * @return rst_code_e RST_OK if successful
     */
    rst_code_e network_download_model(ModelType type, const std::string& model_name, DownloadProgressCallback callback = nullptr) const;

    /**
     * @brief Check if a model is available on the network
     */
    rst_code_e network_is_model_available(ModelType type, const std::string& model_name) const;

    /**
     * @brief Auto-selects the best locally available Whisper model.
     * Order of preference: small -> base -> tiny.
     * Fallback to "tiny" if none found.
     */
    std::string auto_select_whisper_model() const;

    /**
     * @brief Auto-selects the best locally available Llama embedding model.
     * Order of preference: multilingual-e5-large-q8_0 -> multilingual-e5-large-f16 -> first available.
     * Fallback to "multilingual-e5-large-q8_0" if none found.
     */
    std::string auto_select_llama_model() const;

private:
    static constexpr const char* WHISPER_BASE_URL = "https://huggingface.co/ggerganov/whisper.cpp";
    static constexpr const char* WHISPER_URL_PREFIX = "resolve/main";
    static constexpr const char* WHISPER_FILE_PREFIX = "ggml-";
    static constexpr const char* WHISPER_FILE_EXT = ".bin";

    struct ProgressContext {
        std::string file_name;
        DownloadProgressCallback callback;
    };

    static size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream);
    static size_t write_to_string(void *ptr, size_t size, size_t nmemb, std::string *s);
    static int progress_callback(void *clientp, long long dltotal, long long dlnow, long long ultotal, long long ulnow);

    // Resolves model repository URL and filename on Hugging Face
    rst_code_e get_model_download_info(ModelType type, const std::string& model_name, std::string& out_url, std::string& out_filename) const;

    // Helper to query Hugging Face API tree for files in a repository
    rst_code_e fetch_hf_tree_files(const std::string& owner_repo, std::vector<std::string>& out_files) const;
};

#endif // __MANAGER_MODELS_HPP__
