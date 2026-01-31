#include <fstream>
#include <iostream>

#include "tinyfiledialogs.h"

#include "configuration/configuration_system.hpp"
#include "primitives/utils_logger.hpp"
#include "file_handler/file_handler.hpp"

rst_code_e FileHandler::get_file_path_from_user_selection(std::string &file_path)
{
    ConfigurationSystem &config = ConfigurationSystem::getInstance();

    char extensions_buffer[ConfigurationSystem::MAX_EXTENSIONS_COUNT][ConfigurationSystem::MAX_EXTENSIONS_LENGTH] = {};
    int count = config.get_text_files_extensions_allowed(extensions_buffer);
    if (count == 0)
    {
        logger->info("Loaded {} extensions", count);
        return FILE_UPLOAD_ERROR;
    }

    for (int i = 0; i < count; i++)
    {
        logger->info("Loaded extension: {}", extensions_buffer[i]);
    }

    // Create an array of pointers to pass to tinyfd
    const char *lFilterPatterns[ConfigurationSystem::MAX_EXTENSIONS_COUNT];
    for (int i = 0; i < count; i++)
    {
        lFilterPatterns[i] = extensions_buffer[i];
    }

    const char *selection = tinyfd_openFileDialog(
        "Select a text file",
        "",
        count,
        lFilterPatterns,
        "CantaTema files filter",
        0);

    if (selection)
    {
        file_path = std::string(selection);
        logger->info("Selected: {}", file_path);
        return RST_OK;
    }

    logger->error("Selection canceled.");
    return FILE_UPLOAD_ERROR;
}

bool FileHandler::IsPathValid(const std::string& path) const {
    return !path.empty() && std::filesystem::exists(std::filesystem::path(path).parent_path());
}

rst_code_e FileHandler::read_and_stream(const std::string& sourcePath, 
                                 std::function<rst_code_e(const std::vector<char>&)> chunkCallback) {

    if (!std::filesystem::is_regular_file(sourcePath)) {
        logger->error("Error: {} is not a regular file.", sourcePath);
        return FILE_NOT_FOUND;
    }

    if ((std::filesystem::status(sourcePath).permissions() & std::filesystem::perms::owner_read) == std::filesystem::perms::none) {
        logger->error("Error: Missing read permissions for {}.", sourcePath);
        return FILE_READ_ERROR;
    }

    std::ifstream inFile(sourcePath, std::ios::binary);
    
    if (!inFile.is_open()) {
        logger->error("Error: Cannot open {} for reading.", sourcePath);
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

rst_code_e FileHandler::save_chunk(const std::string& destPath, 
                            const std::vector<char>& data, 
                            bool isFirstChunk) {
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

rst_code_e FileHandler::remove_file(const std::string file_path) {
    std::error_code ec;
    if (!std::filesystem::exists(file_path)) {
        logger->warn("File not found for deletion: {}", file_path);
        return RST_OK;
    }

    if (std::filesystem::remove(file_path, ec)) {
        return RST_OK;
    }

    logger->error("Error removing file {}: {}", file_path, ec.message());
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

rst_code_e FileHandler::upload_file(const std::string &destination) {
    logger->error("Not implemented yet.");
    return UNKNOWN;
}
