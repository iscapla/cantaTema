#include <fstream>
#include <iostream>
#include <future>
#include <vector>

#include <SDL3/SDL.h>

#include "configuration/configuration_system.hpp"
#include "primitives/utils_logger.hpp"
#include "file_handler/file_handler.hpp"

FileHandler::FileHandler(void) :
    file_path(std::string()), max_file_size_in_bytes(0)
{
}

FileHandler::FileHandler(const std::string file_path, const std::uintmax_t max_file_size_in_bytes) :
    file_path(file_path), max_file_size_in_bytes(max_file_size_in_bytes)
{
}

rst_code_e FileHandler::get_file_path_from_user_selection(std::string &obtained_file_path)
{
    ConfigurationSystem &config = ConfigurationSystem::getInstance();

    char extensions_buffer[ConfigurationSystem::MAX_EXTENSIONS_COUNT][ConfigurationSystem::MAX_EXTENSIONS_LENGTH] = {};
    int count = config.get_text_files_extensions_allowed(extensions_buffer);
    if (count == 0)
    {
        logger->info("Loaded {} extensions", count);
        return FILE_UPLOAD_ERROR;
    }

    std::string filter_pattern;
    for (int i = 0; i < count; i++)
    {
        std::string ext = extensions_buffer[i];
        // Remove wildcard and dot if present (e.g. "*.txt" -> "txt") for SDL3 format
        size_t last_dot = ext.find_last_of('.');
        if (last_dot != std::string::npos) {
            ext = ext.substr(last_dot + 1);
        }
        
        if (!filter_pattern.empty()) {
            filter_pattern += ";";
        }
        filter_pattern += ext;
        
        logger->info("Loaded extension: {}", extensions_buffer[i]);
    }

    SDL_DialogFileFilter filter;
    filter.name = "CantaTema files filter";
    filter.pattern = filter_pattern.c_str();

    std::promise<std::string> promise;
    std::future<std::string> future = promise.get_future();

    auto callback = [](void* userdata, const char* const* filelist, int filter) {
        auto* p = static_cast<std::promise<std::string>*>(userdata);
        if (filelist && filelist[0]) {
            p->set_value(std::string(filelist[0]));
        } else {
            p->set_value(""); // Cancelled or error
        }
    };

    SDL_ShowOpenFileDialog(callback, &promise, nullptr, &filter, 1, nullptr, false);

    // Wait for the result, pumping events to ensure the callback is dispatched
    while (future.wait_for(std::chrono::milliseconds(10)) != std::future_status::ready) {
        SDL_PumpEvents();
    }

    std::string selection = future.get();

    if (!selection.empty())
    {
        obtained_file_path = selection;
        logger->info("Selected: {}", obtained_file_path);
        return RST_OK;
    }

    logger->error("Selection canceled.");
    return FILE_UPLOAD_ERROR;
}

rst_code_e FileHandler::read_and_stream(std::function<rst_code_e(const std::vector<char>&)> chunkCallback) {

    if(file_path.empty()){
        logger->error("File path is empty");
        return FILE_NOT_FOUND;
    }

    if (!std::filesystem::is_regular_file(file_path)) {
        logger->error("Error: {} is not a regular file.", file_path.string());
        return FILE_NOT_FOUND;
    }

    if ((std::filesystem::status(file_path).permissions() & std::filesystem::perms::owner_read) == std::filesystem::perms::none) {
        logger->error("Error: Missing read permissions for {}.", file_path.string());
        return FILE_READ_ERROR;
    }

    std::ifstream inFile(file_path, std::ios::binary);
    
    if (!inFile.is_open()) {
        logger->error("Error: Cannot open {} for reading.", file_path.string());
        return FILE_READ_ERROR;
    }

    std::vector<char> buffer(CHUNK_SIZE);
    rst_code_e rst = RST_OK;
    
    while (inFile.read(buffer.data(), CHUNK_SIZE) || inFile.gcount() > 0) {
        size_t bytesRead = static_cast<size_t>(inFile.gcount());
        
        // If we read less than the full chunk size, shrink the vector before sending
        if (bytesRead < CHUNK_SIZE) {
            buffer.resize(bytesRead);
        }

        if (chunkCallback) {
            rst = chunkCallback(buffer);
            if (rst != RST_OK) {
                return rst;
            }
        }

        // Restore size for the next iteration
        if (buffer.size() < CHUNK_SIZE) {
            buffer.resize(CHUNK_SIZE);
        }
    }

    return RST_OK;
}

rst_code_e FileHandler::read_range(uint64_t offset, size_t length, std::vector<uint8_t>& out_buffer, bool& out_is_eof) const {
    out_buffer.clear();
    out_is_eof = false;

    if (file_path.empty()) {
        logger->error("File path is empty");
        return FILE_NOT_FOUND;
    }

    if (!std::filesystem::is_regular_file(file_path)) {
        logger->error("Error: {} is not a regular file.", file_path.string());
        return FILE_NOT_FOUND;
    }

    std::error_code ec;
    auto fileSize = std::filesystem::file_size(file_path, ec);
    if (ec) {
        logger->error("Error checking size for {}: {}", file_path.string(), ec.message());
        return FILE_READ_ERROR;
    }

    if (offset >= fileSize) {
        out_is_eof = true;
        return RST_OK;
    }

    std::ifstream inFile(file_path, std::ios::binary);
    if (!inFile.is_open()) {
        logger->error("Error: Cannot open {} for reading.", file_path.string());
        return FILE_READ_ERROR;
    }

    inFile.seekg(static_cast<std::streamoff>(offset));
    if (!inFile) {
        logger->error("Error seeking to offset {} in {}.", offset, file_path.string());
        return FILE_READ_ERROR;
    }

    size_t bytesToRead = length;
    if (offset + bytesToRead >= fileSize) {
        bytesToRead = static_cast<size_t>(fileSize - offset);
        out_is_eof = true;
    }

    if (bytesToRead == 0) {
        return RST_OK;
    }

    out_buffer.resize(bytesToRead);
    inFile.read(reinterpret_cast<char*>(out_buffer.data()), bytesToRead);
    size_t bytesRead = static_cast<size_t>(inFile.gcount());
    if (bytesRead < bytesToRead) {
        out_buffer.resize(bytesRead);
        out_is_eof = true;
    }

    return RST_OK;
}

rst_code_e FileHandler::save_chunk(const std::string& destPath, 
                            const std::vector<char>& data, 
                            bool isFirstChunk)
{

    // If it's the first chunk, we truncate (overwrite). Otherwise, we append.
    std::ios_base::openmode mode = std::ios::binary;
    if (isFirstChunk) {
        mode |= std::ios::trunc;
    } else {
        mode |= std::ios::app;
    }

    std::filesystem::path dest_path_obj(destPath);
    std::filesystem::path parent_path = dest_path_obj.parent_path();

    if (!parent_path.empty() && !std::filesystem::exists(parent_path)) {
        std::error_code ec;
        if (!std::filesystem::create_directories(parent_path, ec)) {
            logger->error("Error creating directories for {}: {}", parent_path.string(), ec.message());
            return FILE_UPLOAD_ERROR;
        }
    }

    std::ofstream outFile(destPath, mode);
    
    if (!outFile.is_open()) {
        logger->error("Error: Cannot open {} for writing.", destPath);
        return FILE_UPLOAD_ERROR;
    }

    outFile.write(data.data(), data.size());
    return outFile.good() ? RST_OK : FILE_UPLOAD_ERROR;
}

rst_code_e FileHandler::remove_file(void) {
    std::error_code ec;

    if(file_path.empty()){
        logger->error("File path is empty");
        return FILE_NOT_FOUND;
    }

    if (!std::filesystem::exists(file_path)) {
        logger->warn("File not found for deletion: {}", file_path.string());
        return RST_OK;
    }

    if (std::filesystem::remove(file_path, ec)) {
        return RST_OK;
    }

    logger->error("Error removing file {}: {}", file_path.string(), ec.message());
    return UNKNOWN;
}

rst_code_e FileHandler::remove_folder(const std::string folder_path){
    if (folder_path != "")
    {
        std::filesystem::path p(folder_path);
        std::error_code ec;
        if (std::filesystem::exists(p))
        {
            std::filesystem::remove_all(p, ec);
            if (ec)
            {
                logger->warn("Error removing directory {}: {}", p.string(), ec.message());
                return SUBJECT_ERROR;
            }
        }else{
            logger->warn("Folder not found for deletion: {}", folder_path);
            return RST_OK;
        }
    }
    return RST_OK;
}

rst_code_e FileHandler::upload_file(const std::string &destination, unsigned int &uploaded_bytes) {
    bool first = true;
    rst_code_e rst = FILE_UPLOAD_ERROR;
    uploaded_bytes = 0;

    std::uintmax_t file_size = get_file_size_in_bytes();
    if(is_file_path_valid() && file_size > max_file_size_in_bytes){
        logger->error("File too big for upload: {}", file_size);
    }else{
        logger->debug("Uploading file of size: {}", file_size);
    }

    rst = read_and_stream([&](const std::vector<char>& chunk) {
        uploaded_bytes += chunk.size();
        // Here you could encrypt the chunk or send via gRPC
        // For this example, we just save it back to a "received" file
        rst_code_e rst = save_chunk(destination, chunk, first);
        first = false;
        return rst;
    });

    if(rst == RST_OK){
        logger->info("File saved into {} (Size: {} bytes)", destination, uploaded_bytes);
    }else{
        logger->error("Impossible to save file into");
        uploaded_bytes = 0;
    }

    return rst;
}

bool FileHandler::is_file_path_valid(void) const {
    if (file_path.empty()) return false;
    // Check if it exists and is not a directory
    return std::filesystem::exists(file_path) && std::filesystem::is_regular_file(file_path);
}

std::uintmax_t FileHandler::get_file_size_in_bytes(void) const {
    if (!is_file_path_valid()) return 0;
    std::error_code ec;
    std::uintmax_t size = std::filesystem::file_size(file_path, ec);
    if (ec) {
        logger->error("Error getting file size for {}: {}", file_path.string(), ec.message());
        return 0;
    }
    return size;
}

std::filesystem::path FileHandler::get_file_path(void) const {
    return file_path;
}
