#ifndef FILE_HANDLER_HPP
#define FILE_HANDLER_HPP

#include <string>
#include <vector>
#include <functional>
#include <filesystem>

#include "primitives/utils_logger.hpp"
#include "primitives/definitions.hpp"

class FileHandler {
protected:
    // 64KB chunks: optimal for most OS disk buffers
    static constexpr size_t CHUNK_SIZE = 65536;

    // Common utility for child classes to validate paths
    bool IsPathValid(const std::string& path) const;

public:
    virtual ~FileHandler() = default;

    /**
     * @brief Opens an OS selection window to select a file.
     * 
     * @param file_path Reference to string where the selected path will be stored.
     * @return rst_code_e RST_OK if successful, error code otherwise.
     */
    static rst_code_e get_file_path_from_user_selection(std::string &file_path);

    /**
     * Reads a file in chunks and passes each chunk to the provided callback.
     * Use this to "send" files to a network or an encryption engine.
     * 
     * @param sourcePath The path to the source file to read.
     * @param chunkCallback A callback function that receives each data chunk.
     * @return bool True if the operation completed successfully, false otherwise.
     */
    virtual rst_code_e read_and_stream(const std::string& sourcePath, 
                               std::function<rst_code_e(const std::vector<char>&)> chunkCallback);

    /**
     * Saves a single chunk of data to a file.
     * Set 'isFirstChunk' to true to overwrite existing files, false to append.
     * 
     * @param destPath The destination file path.
     * @param data The vector containing the data chunk to write.
     * @param isFirstChunk If true, truncates the file before writing; otherwise appends.
     * @return bool True if the write operation was successful, false otherwise.
     */
    virtual rst_code_e save_chunk(const std::string& destPath, 
                           const std::vector<char>& data, 
                           bool isFirstChunk = false);
    
    /**
     * @brief Removes a file from the filesystem.
     * 
     * @param file_path The path to the file to remove.
     * @return rst_code_e RST_OK if successful, FILE_NOT_FOUND if file doesn't exist, or error code.
     */
    virtual rst_code_e remove_file(const std::string file_path);

    /**
     * @brief Removes a folder from the filesystem and all of its content
     * 
     * @param file_path The path to the folder to remove.
     * @return rst_code_e RST_OK if successful, FILE_NOT_FOUND if folder doesn't exist, or error code.
     */
    virtual rst_code_e remove_folder(const std::string folder_path);
    
    /**
     * @brief Uploads the audio file to the specified destination.
     * 
     * @param destination The path where the file should be uploaded/saved.
     * @return rst_code_e RST_OK if successful, error code otherwise.
     */
    virtual rst_code_e upload_file(const std::string &destination);
};

#endif // FILE_HANDLER_HPP