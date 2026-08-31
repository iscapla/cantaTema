/**
 * @file i_file_handler.hpp
 * @brief Abstract interface for filesystem file operations and path verification.
 */

#ifndef I_FILE_HANDLER_HPP
#define I_FILE_HANDLER_HPP

#include <string>
#include <cstdint>
#include <filesystem>
#include "primitives/definitions.hpp"

/**
 * @class IFileHandler
 * @brief Abstract interface defining standard file handling, removal, and property queries.
 */
class IFileHandler {
public:
    /**
     * @brief Virtual destructor for IFileHandler.
     */
    virtual ~IFileHandler() = default;

    /**
     * @brief Removes the underlying file from the filesystem.
     * @return rst_code_e RST_OK on successful deletion, or appropriate error code.
     */
    virtual rst_code_e remove_file() = 0;

    /**
     * @brief Checks whether the specified file path is valid and accessible on disk.
     * @return true if file path exists and is valid, false otherwise.
     */
    virtual bool is_file_path_valid() const = 0;

    /**
     * @brief Retrieves the size of the underlying file in bytes.
     * @return std::uintmax_t Size of file in bytes, or 0 if invalid.
     */
    virtual std::uintmax_t get_file_size_in_bytes() const = 0;

    /**
     * @brief Returns the absolute filesystem path of the managed file.
     * @return std::filesystem::path Path object referencing the file.
     */
    virtual std::filesystem::path get_file_path() const = 0;

    /**
     * @brief Reads a specific byte range directly from the file into memory.
     * @param offset Byte offset to start reading from.
     * @param length Number of bytes to read.
     * @param out_buffer Output vector receiving the bytes.
     * @param out_is_eof Output flag set to true if end-of-file was reached.
     * @return rst_code_e RST_OK on success, FILE_READ_ERROR or FILE_NOT_FOUND on failure.
     */
    virtual rst_code_e read_range(uint64_t offset, size_t length, std::vector<uint8_t>& out_buffer, bool& out_is_eof) const = 0;

    /**
     * @brief Reads the entire file in chunks and passes each chunk to the provided callback.
     * @param chunkCallback A callback function that receives each data chunk.
     * @return rst_code_e RST_OK on success, or appropriate error code.
     */
    virtual rst_code_e read_and_stream(std::function<rst_code_e(const std::vector<char>&)> chunkCallback) = 0;
};

#endif // I_FILE_HANDLER_HPP
