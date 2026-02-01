
#include "file_handler/text_handler.hpp"
#include "configuration/configuration_system.hpp"

TextFileHandler::TextFileHandler(const std::string &file_path)
    : FileHandler(file_path, ConfigurationSystem::getInstance().get_user_default_max_text_file_size_in_mb() * 1024 * 1024) {
}

TextFileHandler::~TextFileHandler(void){}

rst_code_e TextFileHandler::upload_file(const std::string &destination, unsigned int &uploaded_bytes) {
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
