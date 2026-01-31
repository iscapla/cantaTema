#ifndef SOUND_SYSTEM_HPP
#define SOUND_SYSTEM_HPP

#include "miniaudio.h"
#include "opus.h"
#include "sound_system/i_sound_system.hpp"
#include <string>
#include <vector>
#include <atomic>
#include <cstdio>

class SoundSystem : public ISoundSystem {
public:
    /**
     * @brief Constructor. Initializes the miniaudio context with the given configuration.
     * @param config The configuration settings for audio capture and playback.
     */
    SoundSystem(const Config& config);

    /**
     * @brief Destructor. Uninitializes devices and context.
     */
    ~SoundSystem() override;

    /**
     * @brief Retrieves a list of available audio capture devices (microphones).
     * 
     * @return std::vector<DeviceInfo> A vector containing information about each capture device.
     */
    std::vector<DeviceInfo> getCaptureDevices() override;

    // Recording
    /**
     * @brief Starts recording audio to a specified file.
     * 
     * @param filePath The path where the recorded audio will be saved.
     * @param deviceIndex The index of the capture device to use. Pass -1 to use the default device.
     * @return bool True if recording started successfully, false otherwise.
     */
    bool startRecording(const std::string& filePath, int deviceIndex = -1) override;

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
     * @param filePath The path to the audio file to play.
     * @return bool True if playback started successfully, false otherwise.
     */
    bool play(const std::string& filePath) override;

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

private:
    Config m_config;
    ma_context m_context;
    bool m_contextInitialized;

    // Recording State
    ma_device m_captureDevice;
    OpusEncoder* m_opusEncoder;
    FILE* m_recordFile;
    std::vector<float> m_recordBuffer;
    std::atomic<bool> m_isRecording;
    bool m_captureDeviceInitialized;
    std::atomic<ma_uint64> m_recordedFrames;

    // Playback State
    ma_device m_playbackDevice;
    OpusDecoder* m_opusDecoder;
    FILE* m_playFile;
    std::vector<float> m_playBuffer;
    std::atomic<bool> m_isPlaying;
    bool m_playbackDeviceInitialized;
    std::atomic<ma_uint64> m_playedFrames;

    // Internal Callbacks
    static void data_callback_record(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
    static void data_callback_play(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount);
};

#endif //SOUND_SYSTEM_HPP
