#ifndef SOUND_SYSTEM_HPP
#define SOUND_SYSTEM_HPP

#include <SDL3/SDL.h>
#include "opus.h"
#include "sound_system/i_sound_system.hpp"
#include <string>
#include <vector>
#include <atomic>
#include <cstdio>
#include <cstdint>

class SoundSystem : public ISoundSystem {
public:
    /**
     * @brief Constructor. Initializes the SDL3 audio subsystem with the given configuration.
     * @param config The configuration settings for audio capture and playback.
     */
    SoundSystem(const SoundSystemConfig& config);

    /**
     * @brief Destructor. Uninitializes devices and context.
     */
    ~SoundSystem() override;

    /**
     * @brief Retrieves a list of available audio capture devices (microphones).
     * 
     * @return std::vector<SoundSystemDeviceInfo> A vector containing information about each capture device.
     */
    std::vector<SoundSystemDeviceInfo> getCaptureDevices() override;

    // Recording
    /**
     * @brief Starts recording audio to a specified file.
     * 
     * @param fileHandler The file handler containing the path where the recorded audio will be saved.
     * @param deviceIndex The index of the capture device to use. Pass -1 to use the default device.
     * @return bool True if recording started successfully, false otherwise.
     */
    bool startRecording(const SoundFileHandler& fileHandler, int deviceIndex = -1) override;

    /**
     * @brief Stops the current recording session.
     */
    void stopRecording() override;

    /**
     * @brief Checks if the system is currently recording.
     * 
     * @return bool True if recording, false otherwise.
     */
    bool isRecording() const override;

    /**
     * @brief Returns the number of milliseconds that have passed since the record started.
     * 
     * @return unsigned long long Milliseconds since recording started. Returns 0 if not recording.
     */
    unsigned long long get_recording_timestamp() override;

    // Playback
    /**
     * @brief Starts playing an audio file.
     * 
     * @param fileHandler The file handler containing the path to the audio file to play.
     * @param callback Optional callback function to receive playback events (start, stop, end, error, timestamp).
     * @return bool True if playback started successfully, false otherwise.
     */
    bool play(const SoundFileHandler& fileHandler, PlaybackCallback callback = nullptr) override;

    /**
     * @brief Stops the current playback.
     */
    void stopPlaying() override;

    /**
     * @brief Checks if the system is currently playing audio.
     * 
     * @return bool True if playing, false otherwise.
     */
    bool isPlaying() const override;

    /**
     * @brief Returns the number of milliseconds that have passed since the music started playing.
     * 
     * @return unsigned long long Milliseconds since playback started. Returns 0 if not playing.
     */
    unsigned long long get_playing_timestamp() override;

    /**
     * @brief Reads a range of decrypted audio bytes from an audio file.
     * @param fileHandler Sound file handler target.
     * @param offset Byte offset in the file.
     * @param length Number of bytes to read.
     * @param out_buffer Output buffer to receive decrypted bytes.
     * @param out_is_eof Set to true if the end of file was reached.
     * @return rst_code_e RST_OK on success, or error code.
     */
    rst_code_e read_decrypted_audio_range(
        const SoundFileHandler& fileHandler,
        uint64_t offset,
        size_t length,
        std::vector<uint8_t>& out_buffer,
        bool& out_is_eof
    ) override;

private:
    SoundSystemConfig m_config;
    bool m_sdlInitialized;

    // Recording State
    SDL_AudioStream* m_captureStream;
    SDL_AudioDeviceID m_captureDeviceId;
    OpusEncoder* m_opusEncoder;
    FILE* m_recordFile;
    std::vector<float> m_recordBuffer;
    std::atomic<bool> m_isRecording;
    std::atomic<uint64_t> m_recordedFrames;

    // Playback State
    SDL_AudioStream* m_playbackStream;
    SDL_AudioDeviceID m_playbackDeviceId;
    OpusDecoder* m_opusDecoder;
    FILE* m_playFile;
    std::vector<float> m_playBuffer;
    std::atomic<bool> m_isPlaying;
    std::atomic<uint64_t> m_playedFrames;
    PlaybackCallback m_playbackCallback;

    // Internal Callbacks
    static void SDLCALL data_callback_record(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount);
    static void SDLCALL data_callback_play(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount);

    // Ogg and Encryption Helpers
    struct OggStreamState {
        uint32_t serial;
        uint32_t sequence;
        uint64_t granulePos;
    };
    OggStreamState m_oggState;

    static uint32_t update_crc(uint32_t crc, const uint8_t* data, size_t len);
    void write_le32(std::vector<uint8_t>& buf, uint32_t val);
    void write_le64(std::vector<uint8_t>& buf, uint64_t val);
    void write_le16(std::vector<uint8_t>& buf, uint16_t val);
    void write_ogg_page(FILE* file, OggStreamState& state, const uint8_t* packet, int packetLen, int headerType);
    void xor_process(void* data, size_t len, long offset);
    size_t secure_fwrite(const void* ptr, size_t size, size_t count, FILE* stream);
    size_t secure_fread(void* ptr, size_t size, size_t count, FILE* stream);
    std::uintmax_t get_encrypted_file_duration(const std::string& filePath);
};

#endif //SOUND_SYSTEM_HPP
