#ifndef SOUND_CONVERTER_HPP
#define SOUND_CONVERTER_HPP

#include <string>

/**
 * @class SoundConverter
 * @brief Utility class for decoding Opus audio files (encrypted or raw Ogg/Opus) to standard WAV format.
 */
class SoundConverter {
public:
    /**
     * @brief Decodes an Ogg/Opus audio file (encrypted or unencrypted) and writes it to a WAV file.
     * 
     * @param opus_path Path to the input Opus audio file.
     * @param wav_out_path Path where the converted WAV audio file should be saved.
     * @return true if conversion succeeded, false otherwise.
     */
    static bool convert_opus_to_wav(const std::string& opus_path, const std::string& wav_out_path);
};

#endif // SOUND_CONVERTER_HPP
