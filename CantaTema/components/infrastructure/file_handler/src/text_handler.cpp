
#include "file_handler/text_handler.hpp"

TextFileHandler::TextFileHandler(const std::string &file_path) : file_path(file_path) {}

TextFileHandler::~TextFileHandler(void){}

rst_code_e TextFileHandler::upload_file(const std::string &destination) {
        bool first = true;
        rst_code_e rst = FILE_UPLOAD_ERROR;

        rst = read_and_stream(file_path, [&](const std::vector<char>& chunk) {
            // Here you could encrypt the chunk or send via gRPC
            // For this example, we just save it back to a "received" file
            rst_code_e rst = save_chunk(destination, chunk, first);
            first = false;
            return rst;
        });

        if(rst == RST_OK){
            logger->info("File saved into {}", destination);
        }else{
            logger->error("Impossible to save file into");
        }

        return rst;
    }
