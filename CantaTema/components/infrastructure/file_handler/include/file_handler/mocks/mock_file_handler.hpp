#ifndef MOCK_FILE_HANDLER_HPP
#define MOCK_FILE_HANDLER_HPP

#include <gmock/gmock.h>
#include "file_handler/i_file_handler.hpp"

class MockFileHandler : public IFileHandler {
public:
    MOCK_METHOD(rst_code_e, remove_file, (), (override));
    MOCK_METHOD(bool, is_file_path_valid, (), (const, override));
    MOCK_METHOD(std::uintmax_t, get_file_size_in_bytes, (), (const, override));
    MOCK_METHOD(std::filesystem::path, get_file_path, (), (const, override));
    MOCK_METHOD(rst_code_e, read_range, (uint64_t offset, size_t length, std::vector<uint8_t>& out_buffer, bool& out_is_eof), (const, override));
    MOCK_METHOD(rst_code_e, read_and_stream, (std::function<rst_code_e(const std::vector<char>&)> chunkCallback), (override));
};

#endif // MOCK_FILE_HANDLER_HPP
