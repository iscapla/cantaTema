#ifndef SOUND_FILE_HANDLER_HPP
#define SOUND_FILE_HANDLER_HPP

#include <string>

#include "file_handler/file_handler.hpp"

class SoundFileHandler : public FileHandler{

public:
    SoundFileHandler(const std::string &file_path);
    ~SoundFileHandler(void);

    /**
     * @brief Gets the recorded time from the metadata in seconds
     * 
     * @return std::uintmax_t recorded seconds
     */
    std::uintmax_t get_recorded_seconds(void) const;
};

#endif // SOUND_FILE_HANDLER_HPP