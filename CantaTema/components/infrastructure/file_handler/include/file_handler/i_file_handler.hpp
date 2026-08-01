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
};

#endif // I_FILE_HANDLER_HPP
