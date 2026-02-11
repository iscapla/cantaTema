#ifndef ISOUND_SYSTEM_HPP
#define ISOUND_SYSTEM_HPP

#include <string>
#include <vector>
#include <functional>

#include "file_handler/sound_handler.hpp"

class ISoundSystem {
public:
    // TODO: Load the values from the config file
    struct SoundSystemConfig {
        int sampleRate = 48000;
        int channels = 1;
        int frameSize = 960; // 20ms at 48kHz
        std::string encryptionKey = "";
    };

    struct SoundSystemDeviceInfo {
        int index;
        std::string name;
        bool isDefault;
    };

    enum class PlaybackEvent {
        PLAY_START,
        PLAY_END,
        PLAY_STOP,
        PLAY_ERROR,
        PLAY_TIMESTAMP
    };

    using PlaybackCallback = std::function<void(PlaybackEvent, unsigned int)>;

    virtual ~ISoundSystem() = default;

    virtual std::vector<SoundSystemDeviceInfo> getCaptureDevices() = 0;
    virtual bool startRecording(const SoundFileHandler& fileHandler, int deviceIndex = -1) = 0;
    virtual void stopRecording() = 0;
    virtual bool isRecording() const = 0;
    virtual unsigned long long get_recording_timestamp() = 0;
    virtual bool play(const SoundFileHandler& fileHandler, PlaybackCallback callback = nullptr) = 0;
    virtual void stopPlaying() = 0;
    virtual bool isPlaying() const = 0;
    virtual unsigned long long get_playing_timestamp() = 0;
};

#endif // ISOUND_SYSTEM_HPP