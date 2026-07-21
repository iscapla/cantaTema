#ifndef I_FILE_HANDLER_HPP
#define I_FILE_HANDLER_HPP

#include <string>
#include <cstdint>
#include <filesystem>
#include "primitives/definitions.hpp"

class IFileHandler {
public:
    virtual ~IFileHandler() = default;
    virtual rst_code_e remove_file() = 0;
    virtual bool is_file_path_valid() const = 0;
    virtual std::uintmax_t get_file_size_in_bytes() const = 0;
    virtual std::filesystem::path get_file_path() const = 0;
};

#endif // I_FILE_HANDLER_HPP
