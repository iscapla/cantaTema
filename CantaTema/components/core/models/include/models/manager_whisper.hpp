#ifndef __MANAGER_WHISPER_HPP__
#define __MANAGER_WHISPER_HPP__

#include "primitives/definitions.hpp"
#include <string>
#include <vector>
#include <functional>
#include <cstdio>

struct DownloadProgress {
    std::string file_name;
    size_t total_bytes;
    size_t downloaded_bytes;
};

using DownloadProgressCallback = std::function<void(const DownloadProgress&)>;

class ManagerWhisper{

public:

    struct WhisperModel {
        std::string name;
        std::string path;
        bool available_local;
        bool available_network;
    };

    /**
     * @brief Construct a new Manager Whisper object
     * 
     */
    ManagerWhisper();

    /**
     * @brief Destroy the Manager Whisper object
     * 
     */
    ~ManagerWhisper();

    /**
     * @brief Get the available models object
     * 
     * @param check_network If true, it will check the network for available models
     * @param models Vector where the models will be stored
     * @return rst_code_e RST_OK if successful
     */
    rst_code_e get_available_models(bool check_network, std::vector<WhisperModel> &models) const;

    /**
     * @brief Get the available models from network object
     * 
     * @param models Vector where the models names will be stored
     * @return rst_code_e RST_OK if successful
     */
    rst_code_e network_get_available_models(std::vector<std::string> &models) const;

    /**
     * @brief Download a model from the network
     * 
     * @param model_name Name of the model to download
     * @param callback Callback function to report progress
     * @return rst_code_e RST_OK if successful
     */
    rst_code_e network_download_model(const std::string model_name, DownloadProgressCallback callback = nullptr) const;

    /**
     * @brief Check if a model is available in the network
     * 
     * @param model_name Name of the model to check
     * @return rst_code_e RST_OK if available
     */
    rst_code_e network_is_model_available(const std::string model_name) const;

    /**
     * @brief Get the available models from local storage
     * 
     * @param models Vector where the models names will be stored
     * @return rst_code_e RST_OK if successful
     */
    rst_code_e local_get_available_models(std::vector<std::string> &models) const;

    /**
     * @brief Check if a model is available locally
     * 
     * @param model_name Name of the model to check
     * @return rst_code_e RST_OK if available
     */
    rst_code_e local_is_model_available(const std::string model_name) const;

private:
    static constexpr const char* WHISPER_BASE_URL = "https://huggingface.co/ggerganov/whisper.cpp";
    static constexpr const char* WHISPER_URL_PREFIX = "resolve/main";
    static constexpr const char* WHISPER_FILE_PREFIX = "ggml-";
    static constexpr const char* WHISPER_FILE_EXT = ".bin";

    struct ProgressContext {
        std::string file_name;
        DownloadProgressCallback callback;
    };

    /**
     * @brief Helper function for libcurl to write received data to a file
     * 
     * @param ptr Pointer to the data
     * @param size Size of each element
     * @param nmemb Number of elements
     * @param stream File stream to write to
     * @return size_t Number of bytes written
     */
    static size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream);

    /**
     * @brief Helper function for libcurl to write received data to a string
     * 
     * @param ptr Pointer to the data
     * @param size Size of each element
     * @param nmemb Number of elements
     * @param s String to write to
     * @return size_t Number of bytes written
     */
    static size_t write_to_string(void *ptr, size_t size, size_t nmemb, std::string *s);

    /**
     * @brief Callback function for libcurl to report progress
     * 
     * @param clientp Pointer to the user data
     * @param dltotal Total bytes to download
     * @param dlnow Downloaded bytes
     * @param ultotal Total bytes to upload
     * @param ulnow Uploaded bytes
     * @return int 0 if successful
     */
    static int progress_callback(void *clientp, long long dltotal, long long  dlnow, long long  ultotal, long long  ulnow);

};

#endif //__MANAGER_WHISPER_HPP__