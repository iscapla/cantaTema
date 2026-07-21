#include "models/manager_models.hpp"
#include "primitives/utils_logger.hpp"
#include "primitives/tool_paths.hpp"
#include <curl/curl.h>
#include <iostream>
#include <cstdio>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <map>
#include <fmt/format.h>

ManagerModels::ManagerModels() {}
ManagerModels::~ManagerModels() {}

rst_code_e ManagerModels::get_available_models(bool check_network, std::vector<ModelInfo> &models) const {
    models.clear();
    std::vector<ModelInfo> whisper_models;
    std::vector<ModelInfo> llama_models;

    rst_code_e rc1 = get_whisper_models(check_network, whisper_models);
    rst_code_e rc2 = get_llama_models(check_network, llama_models);

    models.insert(models.end(), whisper_models.begin(), whisper_models.end());
    models.insert(models.end(), llama_models.begin(), llama_models.end());

    return (rc1 == RST_OK && rc2 == RST_OK) ? RST_OK : MODELS_FILE_DOWNLOAD_FAIL;
}

rst_code_e ManagerModels::get_whisper_models(bool check_network, std::vector<ModelInfo> &models) const {
    models.clear();
    std::map<std::string, ModelInfo> model_map;

    // 1. Local models
    std::vector<std::string> local_names;
    try {
        for (const auto & entry : std::filesystem::directory_iterator(ToolPath::get_path_for_models_whisper())) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                std::string prefix = WHISPER_FILE_PREFIX;
                std::string ext = WHISPER_FILE_EXT;
                if (filename.length() > prefix.length() + ext.length() &&
                    filename.compare(0, prefix.length(), prefix) == 0 &&
                    filename.compare(filename.length() - ext.length(), ext.length(), ext) == 0) {
                    std::string name = filename.substr(prefix.length(), filename.length() - prefix.length() - ext.length());
                    ModelInfo info;
                    info.type = ModelType::Whisper;
                    info.name = name;
                    info.available_local = true;
                    info.path = entry.path().string();
                    model_map[name] = info;
                }
            }
        }
    } catch (...) {
        // Directory might not exist or be empty
    }

    // 2. Network models (predefined common Whisper models or network scan)
    std::vector<std::string> network_names = {"tiny", "base", "small", "medium", "large-v3"};
    if (check_network) {
        CURL *curl = curl_easy_init();
        if (curl) {
            std::string html_content;
            curl_easy_setopt(curl, CURLOPT_URL, WHISPER_BASE_URL);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "CantaTema/1.0");
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &html_content);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
            CURLcode res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);
            if (res == CURLE_OK) {
                std::vector<std::string> parsed_names;
                size_t pos = html_content.find(">Model<");
                if (pos != std::string::npos) {
                    while (true) {
                        size_t tr_start = html_content.find("<tr", pos);
                        if (tr_start == std::string::npos) break;
                        size_t tr_end = html_content.find("</tr>", tr_start);
                        if (tr_end == std::string::npos) break;
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
                                if (!name.empty() &&
                                    name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-_") == std::string::npos &&
                                    name.find(".en") == std::string::npos) {
                                    parsed_names.push_back(name);
                                }
                            }
                        }
                        pos = tr_end;
                    }
                }
                if (!parsed_names.empty()) {
                    network_names = parsed_names;
                }
            }
        }
    }

    for (const auto &name : network_names) {
        if (model_map.find(name) != model_map.end()) {
            model_map[name].available_network = true;
        } else {
            ModelInfo info;
            info.type = ModelType::Whisper;
            info.name = name;
            info.available_local = false;
            info.available_network = true;
            info.path = "";
            model_map[name] = info;
        }
    }

    for (const auto &pair : model_map) {
        models.push_back(pair.second);
    }
    return RST_OK;
}

rst_code_e ManagerModels::get_llama_models(bool check_network, std::vector<ModelInfo> &models) const {
    models.clear();
    std::map<std::string, ModelInfo> model_map;

    // 1. Local Llama GGUF models
    try {
        for (const auto & entry : std::filesystem::directory_iterator(ToolPath::get_path_for_models_llama())) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                if (filename.length() > 5 && filename.compare(filename.length() - 5, 5, ".gguf") == 0) {
                    std::string name = filename.substr(0, filename.length() - 5);
                    ModelInfo info;
                    info.type = ModelType::Llama;
                    info.name = name;
                    info.available_local = true;
                    info.path = entry.path().string();
                    model_map[name] = info;
                }
            }
        }
    } catch (...) {
        // Directory empty or doesn't exist
    }

    // 2. Predefined Llama network models
    std::vector<std::string> network_names = {"multilingual-e5-large-q8_0", "multilingual-e5-large-f16"};
    for (const auto &name : network_names) {
        if (model_map.find(name) != model_map.end()) {
            model_map[name].available_network = true;
        } else {
            ModelInfo info;
            info.type = ModelType::Llama;
            info.name = name;
            info.available_local = false;
            info.available_network = true;
            info.path = "";
            model_map[name] = info;
        }
    }

    for (const auto &pair : model_map) {
        models.push_back(pair.second);
    }
    return RST_OK;
}

rst_code_e ManagerModels::local_is_whisper_model_available(const std::string& model_name) const {
    std::string filename = fmt::format("{}{}{}", WHISPER_FILE_PREFIX, model_name, WHISPER_FILE_EXT);
    std::filesystem::path file_path = ToolPath::get_path_for_models_whisper() / filename;
    return std::filesystem::exists(file_path) ? RST_OK : MODELS_FILE_NOT_FOUND;
}

rst_code_e ManagerModels::local_is_llama_model_available(const std::string& model_name) const {
    std::string filename = model_name;
    if (filename.find(".gguf") == std::string::npos) {
        filename += ".gguf";
    }
    std::filesystem::path file_path = ToolPath::get_path_for_models_llama() / filename;
    return std::filesystem::exists(file_path) ? RST_OK : MODELS_FILE_NOT_FOUND;
}

rst_code_e ManagerModels::network_is_model_available(ModelType type, const std::string& model_name) const {
    std::string url, filename;
    if (get_model_download_info(type, model_name, url, filename) != RST_OK) {
        return MODELS_FILE_NOT_FOUND;
    }

    CURL *curl = curl_easy_init();
    rst_code_e ret = MODELS_FILE_NOT_FOUND;
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "CantaTema/1.0");
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
        // Use GET with Range: 0-0 to support Hugging Face LFS signed redirect links
        curl_easy_setopt(curl, CURLOPT_RANGE, "0-0");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_string);
        std::string dummy;
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &dummy);

        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            long response_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
            if (response_code >= 200 && response_code < 400) {
                ret = RST_OK;
            } else {
                logger->error("Network availability check returned HTTP code {} for URL {}", response_code, url);
            }
        } else {
            logger->error("Network availability check perform failed for URL {}: {}", url, curl_easy_strerror(res));
        }
        curl_easy_cleanup(curl);
    }
    return ret;
}

rst_code_e ManagerModels::network_download_model(ModelType type, const std::string& model_name, DownloadProgressCallback callback) const {
    std::string url, filename;
    if (get_model_download_info(type, model_name, url, filename) != RST_OK) {
        return MODELS_FILE_DOWNLOAD_FAIL;
    }

    std::filesystem::path dest_dir = (type == ModelType::Whisper) ? 
        ToolPath::get_path_for_models_whisper() : ToolPath::get_path_for_models_llama();
    
    std::filesystem::path out_path = dest_dir / filename;
    std::filesystem::path tmp_path = dest_dir / (filename + ".tmp");

    CURL *curl = curl_easy_init();
    if (!curl) {
        return MODELS_FILE_DOWNLOAD_FAIL;
    }

    FILE *fp = fopen(tmp_path.string().c_str(), "wb");
    if (!fp) {
        logger->error("Failed to open temp file {} for writing.", tmp_path.string());
        curl_easy_cleanup(curl);
        return MODELS_FILE_DOWNLOAD_FAIL;
    }

    ProgressContext prog_ctx;
    prog_ctx.file_name = filename;
    prog_ctx.callback = callback;

    if (callback) {
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &prog_ctx);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "CantaTema/1.0");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

    logger->info("Downloading {} from {}...", model_name, url);
    CURLcode res = curl_easy_perform(curl);
    fclose(fp);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        logger->error("Download failed: {}", curl_easy_strerror(res));
        std::error_code ec;
        std::filesystem::remove(tmp_path, ec); // clean up partial download
        return MODELS_FILE_DOWNLOAD_FAIL;
    }

    // Rename temp file to output path
    std::error_code ec;
    std::filesystem::rename(tmp_path, out_path, ec);
    if (ec) {
        logger->error("Failed to rename temp download file to {}: {}", out_path.string(), ec.message());
        std::filesystem::remove(tmp_path, ec);
        return MODELS_FILE_DOWNLOAD_FAIL;
    }

    logger->info("Model successfully saved to {}", out_path.string());
    return RST_OK;
}

std::string ManagerModels::auto_select_whisper_model() const {
    std::vector<std::string> preferences = {"small", "base", "tiny"};
    for (const auto &pref : preferences) {
        if (local_is_whisper_model_available(pref) == RST_OK) {
            return pref;
        }
    }

    // Fallback: list all local Whisper files, pick the first
    std::vector<ModelInfo> local_models;
    if (get_whisper_models(false, local_models) == RST_OK) {
        for (const auto &info : local_models) {
            if (info.available_local) {
                return info.name;
            }
        }
    }
    return "tiny"; // Default network model name to download
}

std::string ManagerModels::auto_select_llama_model() const {
    std::vector<std::string> preferences = {"multilingual-e5-large-q8_0", "multilingual-e5-large-f16"};
    for (const auto &pref : preferences) {
        if (local_is_llama_model_available(pref) == RST_OK) {
            return pref;
        }
    }

    // Fallback: list local Llama models, pick first
    std::vector<ModelInfo> local_models;
    if (get_llama_models(false, local_models) == RST_OK) {
        for (const auto &info : local_models) {
            if (info.available_local) {
                return info.name;
            }
        }
    }
    return "multilingual-e5-large-q8_0"; // Default network model name to download
}

size_t ManagerModels::write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    return fwrite(ptr, size, nmemb, stream);
}

size_t ManagerModels::write_to_string(void *ptr, size_t size, size_t nmemb, std::string *s) {
    s->append(static_cast<char *>(ptr), size * nmemb);
    return size * nmemb;
}

int ManagerModels::progress_callback(void *clientp, long long dltotal, long long dlnow, long long ultotal, long long ulnow) {
    ProgressContext *ctx = static_cast<ProgressContext *>(clientp);
    if (ctx && ctx->callback) {
        DownloadProgress progress{ctx->file_name, static_cast<size_t>(dltotal), static_cast<size_t>(dlnow)};
        ctx->callback(progress);
    }
    return 0;
}

rst_code_e ManagerModels::get_model_download_info(ModelType type, const std::string& model_name, std::string& out_url, std::string& out_filename) const {
    if (type == ModelType::Whisper) {
        out_filename = fmt::format("{}{}{}", WHISPER_FILE_PREFIX, model_name, WHISPER_FILE_EXT);
        out_url = fmt::format("{}/{}/{}", WHISPER_BASE_URL, WHISPER_URL_PREFIX, out_filename);
        return RST_OK;
    } else {
        std::string repo = "Zenabius/multilingual-e5-large-Q8_0-GGUF";
        std::string file = "multilingual-e5-large-q8_0.gguf";

        if (model_name == "multilingual-e5-large-q4_k_m") {
            repo = "phate334/multilingual-e5-large-gguf";
            file = "multilingual-e5-large-q4_k_m.gguf";
        } else if (model_name == "multilingual-e5-large-f16") {
            repo = "Zenabius/multilingual-e5-large-Q8_0-GGUF"; // Fallback to Q8 if F16 is not in Zenabius
            file = "multilingual-e5-large-q8_0.gguf";
        } else if (model_name == "multilingual-e5-large-q8_0") {
            repo = "Zenabius/multilingual-e5-large-Q8_0-GGUF";
            file = "multilingual-e5-large-q8_0.gguf";
        } else if (model_name == "multilingual-e5-large") {
            repo = "Zenabius/multilingual-e5-large-Q8_0-GGUF";
            file = "multilingual-e5-large-q8_0.gguf";
        } else if (model_name.find('/') != std::string::npos) {
            // format: owner/repo/filename.gguf
            size_t last_slash = model_name.rfind('/');
            repo = model_name.substr(0, last_slash);
            file = model_name.substr(last_slash + 1);
        } else {
            file = model_name;
            if (file.find(".gguf") == std::string::npos) {
                file += ".gguf";
            }
        }
        out_filename = file;
        out_url = fmt::format("https://huggingface.co/{}/resolve/main/{}", repo, file);
        return RST_OK;
    }
}
