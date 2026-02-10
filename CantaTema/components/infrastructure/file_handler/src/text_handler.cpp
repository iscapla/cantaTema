
#include "file_handler/text_handler.hpp"
#include "configuration/configuration_system.hpp"

TextFileHandler::TextFileHandler(const std::string &file_path)
    : FileHandler(file_path, ConfigurationSystem::getInstance().get_user_default_max_text_file_size_in_mb() * 1024 * 1024) {
}

TextFileHandler::~TextFileHandler(void){}
