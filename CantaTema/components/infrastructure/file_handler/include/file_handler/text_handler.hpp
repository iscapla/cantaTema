#ifndef TEXT_FILE_HANDLER_HPP
#define TEXT_FILE_HANDLER_HPP

#include <string>

#include "file_handler/file_handler.hpp"

class TextFileHandler : public FileHandler{

public:
    TextFileHandler(const std::string &file_path);
    ~TextFileHandler(void);

};

#endif // TEXT_FILE_HANDLER_HPP