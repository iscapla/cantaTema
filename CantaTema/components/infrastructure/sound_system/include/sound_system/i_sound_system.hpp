/**
 * @file i_sound_system.hpp
 * @brief Abstract interface for audio recording, playback, and device management using SDL3 and Opus.
 */

#ifndef ISOUND_SYSTEM_HPP
#define ISOUND_SYSTEM_HPP

#include <string>
#include <vector>
#include <functional>

#include "file_handler/sound_handler.hpp"

/**
 * @class ISoundSystem
 * @brief Abstract interface defining audio capture, playback, timestamp tracking, and device enumeration.
 */
class ISoundSystem {
public:
    /**
     * @struct SoundSystemConfig
     * @brief Configuration parameters for audio recording and encoding.
     */
    struct SoundSystemConfig {
        int sampleRate = 48000;         ///< Audio sampling rate in Hertz (default 48000 Hz).
        int channels = 1;               ///< Number of audio channels (1 = mono).
        int frameSize = 960;            ///< Frame size in samples (960 = 20ms at 48kHz).
        std::string encryptionKey = ""; ///< Custom XOR encryption key for audio file storage.
    };

    /**
     * @struct SoundSystemDeviceInfo
     * @brief Metadata describing an audio capture or playback device.
     */
    struct SoundSystemDeviceInfo {
        int index;          ///< Unique device index.
        std::string name;   ///< Human-readable device name.
        bool isDefault;     ///< True if this is the OS default device.
    };

    /**
     * @enum PlaybackEvent
     * @brief Event types emitted during audio playback progress.
     */
    enum class PlaybackEvent {
        PLAY_START,     ///< Emitted when playback starts.
        PLAY_END,       ///< Emitted when playback reaches end of stream.
        PLAY_STOP,      ///< Emitted when playback is explicitly stopped.
        PLAY_ERROR,     ///< Emitted if an error occurs during playback.
        PLAY_TIMESTAMP  ///< Periodic timestamp progress updates.
    };

    /// Callback signature for audio playback events.
    using PlaybackCallback = std::function<void(PlaybackEvent, unsigned int)>;

    /**
     * @brief Virtual destructor for ISoundSystem.
     */
    virtual ~ISoundSystem() = default;

    /**
     * @brief Enumerates available audio capture devices on the system.
     * @return std::vector<SoundSystemDeviceInfo> List of available input devices.
     */
    virtual std::vector<SoundSystemDeviceInfo> getCaptureDevices() = 0;

    /**
     * @brief Starts recording audio to the specified sound file handler.
     * @param fileHandler Sound file handler target where audio bytes are saved.
     * @param deviceIndex Index of the target capture device (-1 for default device).
     * @return true if recording started successfully, false on error.
     */
    virtual bool startRecording(const SoundFileHandler& fileHandler, int deviceIndex = -1) = 0;

    /**
     * @brief Stops the active recording process and flushes remaining audio data to disk.
     */
    virtual void stopRecording() = 0;

    /**
     * @brief Checks whether audio recording is currently active.
     * @return true if actively recording, false otherwise.
     */
    virtual bool isRecording() const = 0;

    /**
     * @brief Gets the current recording duration timestamp in milliseconds.
     * @return unsigned long long Elapsed recording time in milliseconds.
     */
    virtual unsigned long long get_recording_timestamp() = 0;

    /**
     * @brief Starts audio playback from the provided sound file handler.
     * @param fileHandler Source sound file handler to read audio from.
     * @param callback Optional progress callback for playback state notifications.
     * @return true if playback started successfully, false otherwise.
     */
    virtual bool play(const SoundFileHandler& fileHandler, PlaybackCallback callback = nullptr) = 0;

    /**
     * @brief Stops the current audio playback stream.
     */
    virtual void stopPlaying() = 0;

    /**
     * @brief Checks whether audio playback is currently active.
     * @return true if actively playing audio, false otherwise.
     */
    virtual bool isPlaying() const = 0;

    /**
     * @brief Gets the current playback duration timestamp in milliseconds.
     * @return unsigned long long Elapsed playback time in milliseconds.
     */
    virtual unsigned long long get_playing_timestamp() = 0;
};

#endif // ISOUND_SYSTEM_HPP