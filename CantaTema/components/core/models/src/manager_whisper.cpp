#include "models/manager_whisper.hpp"
#include "primitives/utils_logger.hpp"
#include "primitives/tool_paths.hpp"
#include <curl/curl.h>
#include <iostream>
#include <cstdio>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <map>

ManagerWhisper::ManagerWhisper() {}
ManagerWhisper::~ManagerWhisper() {}

rst_code_e ManagerWhisper::get_available_models(bool check_network, std::vector<WhisperModel> &models) const {
    models.clear();
    std::map<std::string, WhisperModel> model_map;

    // 1. Get local models
    std::vector<std::string> local_models;
    if (local_get_available_models(local_models) == RST_OK) {
        for (const auto &name : local_models) {
            WhisperModel model;
            model.name = name;
            model.available_local = true;
            model.available_network = false; // Will be updated if network check is performed
            
            std::string filename = fmt::format("{}{}{}", WHISPER_FILE_PREFIX, name, WHISPER_FILE_EXT);
            model.path = (ToolPath::get_path_for_models_whisper() / filename).string();
            
            model_map[name] = model;
        }
    }

    // 2. Get network models if requested
    if (check_network) {
        std::vector<std::string> network_models;
        if (network_get_available_models(network_models) == RST_OK) {
            for (const auto &name : network_models) {
                if (model_map.find(name) != model_map.end()) {
                    model_map[name].available_network = true;
                } else {
                    WhisperModel model;
                    model.name = name;
                    model.available_local = false;
                    model.available_network = true;
                    model.path = "";
                    model_map[name] = model;
                }
            }
        }
    }

    // 3. Convert map to vector
    for (const auto &pair : model_map) {
        models.push_back(pair.second);
    }

    return RST_OK;
}

rst_code_e ManagerWhisper::network_get_available_models(std::vector<std::string> &models) const {
    CURL *curl = curl_easy_init();
    if (!curl) {
        logger->error("Failed to initialize curl for model list retrieval.");
        return MODELS_FILE_DOWNLOAD_FAIL;
    }

    std::string html_content;
    std::string url = WHISPER_BASE_URL;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "CantaTema/1.0");
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html_content);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

    logger->debug("Starting network query to fetch available models from {}", url);
    CURLcode res = curl_easy_perform(curl);
    logger->debug("Network query finished.");
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        logger->error("Failed to fetch model list: {}", curl_easy_strerror(res));
        return MODELS_FILE_DOWNLOAD_FAIL;
    }

    models.clear();

    // Find the table header "Model"
    size_t pos = html_content.find(">Model<");
    if (pos == std::string::npos) {
        logger->error("Could not find model table in response.");
        return MODELS_FILE_DOWNLOAD_FAIL;
    }

    // Iterate through rows after the header
    while (true) {
        size_t tr_start = html_content.find("<tr", pos);
        if (tr_start == std::string::npos) break;

        size_t tr_end = html_content.find("</tr>", tr_start);
        if (tr_end == std::string::npos) break;

        // Find the first cell
        size_t td_start = html_content.find("<td", tr_start);
        if (td_start != std::string::npos && td_start < tr_end) {
            size_t content_start = html_content.find(">", td_start) + 1;
            size_t td_end = html_content.find("</td>", content_start);
            
            if (td_end != std::string::npos) {
                std::string raw = html_content.substr(content_start, td_end - content_start);
                std::string name;
                bool in_tag = false;
                for (char c : raw) {
                    if (c == '<') in_tag = true;
                    else if (c == '>') in_tag = false;
                    else if (!in_tag && !std::isspace(static_cast<unsigned char>(c))) name += c;
                }
                if (
                    !name.empty() &&
                    name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-_") == std::string::npos &&
                    name.find(".en") == std::string::npos
                ) {
                    models.push_back(name);
                }
            }
        }
        pos = tr_end;
    }

    return models.empty() ? MODELS_FILE_DOWNLOAD_FAIL : RST_OK;
}

rst_code_e ManagerWhisper::network_download_model(const std::string model_name, DownloadProgressCallback callback) const {

    CURL *curl;
    FILE *fp;
    CURLcode res;
    rst_code_e ret = RST_OK;

    // Construct the URL and output filename
    std::string file_name = fmt::format("{}{}{}", WHISPER_FILE_PREFIX, model_name, WHISPER_FILE_EXT);
    std::string url = fmt::format("{}/{}/{}", WHISPER_BASE_URL, WHISPER_URL_PREFIX, file_name);
    std::string out_filename = (ToolPath::get_path_for_models_whisper() / file_name).string();

    curl = curl_easy_init();
    if (curl) {
        fp = fopen(out_filename.c_str(), "wb");
        if (!fp) {
            logger->error("Failed to open file {} for writing.", out_filename);
            curl_easy_cleanup(curl);
            return MODELS_FILE_DOWNLOAD_FAIL;
        }

        ProgressContext prog_ctx;
        prog_ctx.file_name = out_filename;
        prog_ctx.callback = callback;

        if (callback) {
            curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
            curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &prog_ctx);
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        }

        // Set curl options
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "CantaTema/1.0");
        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L); // Follow redirects (HuggingFace uses redirects)
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);    // Fail on HTTP 4xx/5xx errors
        // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);     // Uncomment for debug output

        logger->debug("Starting download for model: {}", model_name);
        logger->info("Downloading {} from {}...", model_name, url);
        res = curl_easy_perform(curl);
        logger->debug("Finished download for model: {}", model_name);

        if (res != CURLE_OK) {
            logger->error("Download failed: {}", curl_easy_strerror(res));
            ret = MODELS_FILE_DOWNLOAD_FAIL;
        } else {
            logger->info("Model saved to {}", out_filename);
        }

        fclose(fp);
        curl_easy_cleanup(curl);
    }

    return ret;
}

rst_code_e ManagerWhisper::network_is_model_available(const std::string model_name) const {

    CURL *curl = curl_easy_init();
    rst_code_e ret = MODELS_FILE_NOT_FOUND;

    if (curl) {
        std::string file_name = fmt::format("{}{}{}", WHISPER_FILE_PREFIX, model_name, WHISPER_FILE_EXT);
        std::string url = fmt::format("{}/{}/{}", WHISPER_BASE_URL, WHISPER_URL_PREFIX, file_name);

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "CantaTema/1.0");
        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L); // Perform a HEAD request
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        logger->debug("Checking availability for model: {}", model_name);
        CURLcode res = curl_easy_perform(curl);
        logger->debug("Availability check finished for model: {}", model_name);
        if (res == CURLE_OK) {
            long response_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            if (response_code >= 200 && response_code < 400) {
                ret = RST_OK;
            } else {
                logger->warn("Availability check HTTP status {} for model: {}", response_code, model_name);
            }
        } else {
            logger->error("Availability check failed for model {}: {} (code {})", model_name, curl_easy_strerror(res), static_cast<int>(res));
        }

        curl_easy_cleanup(curl);
    }
    return ret;
}

rst_code_e ManagerWhisper::local_get_available_models(std::vector<std::string> &models) const {
    models.clear();
    try {
        for (const auto & entry : std::filesystem::directory_iterator(ToolPath::get_path_for_models_whisper())) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                std::string prefix = WHISPER_FILE_PREFIX;
                std::string ext = WHISPER_FILE_EXT;

                if (filename.length() > prefix.length() + ext.length() &&
                    filename.compare(0, prefix.length(), prefix) == 0 &&
                    filename.compare(filename.length() - ext.length(), ext.length(), ext) == 0) {
                    
                    models.push_back(filename.substr(prefix.length(), filename.length() - prefix.length() - ext.length()));
                }
            }
        }
    } catch (const std::exception& e) {
        logger->error("Failed to list local models: {}", e.what());
        return MODELS_FILE_NOT_FOUND;
    }
    return RST_OK;
}

rst_code_e ManagerWhisper::local_is_model_available(const std::string model_name) const {
    std::string filename = fmt::format("{}{}{}", WHISPER_FILE_PREFIX, model_name, WHISPER_FILE_EXT);
    std::filesystem::path file_path = ToolPath::get_path_for_models_whisper() / filename;
    return std::filesystem::exists(file_path) ? RST_OK : MODELS_FILE_NOT_FOUND;
}

size_t ManagerWhisper::write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

size_t ManagerWhisper::write_to_string(void *ptr, size_t size, size_t nmemb, std::string *s) {
    s->append(static_cast<char *>(ptr), size * nmemb);
    return size * nmemb;
}

int ManagerWhisper::progress_callback(void *clientp, long long  dltotal, long long  dlnow, long long  ultotal, long long  ulnow) {
    ProgressContext *ctx = static_cast<ProgressContext *>(clientp);
    if (ctx && ctx->callback) {
        DownloadProgress progress{ctx->file_name, static_cast<size_t>(dltotal), static_cast<size_t>(dlnow)};
        ctx->callback(progress);
    }
    return 0;
}