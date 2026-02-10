
#include "file_handler/sound_handler.hpp"
#include "configuration/configuration_system.hpp"
#include <taglib/fileref.h>

SoundFileHandler::SoundFileHandler(const std::string &file_path)
    : FileHandler(file_path, ConfigurationSystem::getInstance().get_user_default_max_sound_file_size_in_mb() * 1024 * 1024) {
}

SoundFileHandler::~SoundFileHandler(void){}

std::uintmax_t SoundFileHandler::get_recorded_seconds(void) const {
    if (!is_file_path_valid()) {
        return 0;
    }

    TagLib::FileRef f(get_file_path().string().c_str());

    if (!f.isNull() && f.audioProperties()) {
        return f.audioProperties()->lengthInSeconds();
    }

    return 0;
}
