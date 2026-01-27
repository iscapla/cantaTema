#include <iostream>

#include "primitives/utils_logger.hpp"
#include "sound_system/sound_system.hpp"


SoundSystem::SoundSystem() 
    : m_contextInitialized(false), 
      m_isRecording(false), 
      m_captureDeviceInitialized(false),
      m_recordedFrames(0),
      m_isPlaying(false),
      m_playbackDeviceInitialized(false),
      m_playedFrames(0)
{
    if (ma_context_init(NULL, 0, NULL, &m_context) == MA_SUCCESS) {
        m_contextInitialized = true;
    } else {
        logger->error("[SoundSystem] Failed to initialize context.");
    }
}

SoundSystem::~SoundSystem() {
    stopRecording();
    stopPlaying();
    if (m_contextInitialized) {
        ma_context_uninit(&m_context);
    }
}

std::vector<SoundSystem::DeviceInfo> SoundSystem::getCaptureDevices() {
    std::vector<DeviceInfo> devices;
    if (!m_contextInitialized) return devices;

    ma_device_info* pCaptureInfos;
    ma_uint32 captureCount;
    ma_device_info* pPlaybackInfos;
    ma_uint32 playbackCount;

    if (ma_context_get_devices(&m_context, &pPlaybackInfos, &playbackCount, &pCaptureInfos, &captureCount) == MA_SUCCESS) {
        for (ma_uint32 i = 0; i < captureCount; ++i) {
            devices.push_back({(int)i, pCaptureInfos[i].name, (bool)pCaptureInfos[i].isDefault});
        }
    }
    return devices;
}

void SoundSystem::data_callback_record(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    (void)pOutput;
    if (pInput == nullptr) return;
    
    SoundSystem* pSystem = (SoundSystem*)pDevice->pUserData;
    if (pSystem == nullptr) return;

    ma_encoder_write_pcm_frames(&pSystem->m_encoder, pInput, frameCount, nullptr);
    pSystem->m_recordedFrames += frameCount;
}

bool SoundSystem::startRecording(const std::string& filePath, int deviceIndex) {
    if (!m_contextInitialized) return false;
    if (m_isRecording) stopRecording();

    // 1. Configure Encoder (WAV File)
    ma_encoder_config encoderConfig = ma_encoder_config_init(ma_encoding_format_wav, ma_format_f32, 1, 44100);
    if (ma_encoder_init_file(filePath.c_str(), &encoderConfig, &m_encoder) != MA_SUCCESS) {
        logger->error("[SoundSystem] Failed to initialize output file: {}", filePath);
        return false;
    }

    // 2. Select Device
    ma_device_id* pDeviceID = NULL;
    if (deviceIndex >= 0) {
        ma_device_info* pCaptureInfos;
        ma_uint32 captureCount;
        ma_device_info* pPlaybackInfos;
        ma_uint32 playbackCount;
        if (ma_context_get_devices(&m_context, &pPlaybackInfos, &playbackCount, &pCaptureInfos, &captureCount) == MA_SUCCESS) {
            if ((ma_uint32)deviceIndex < captureCount) {
                pDeviceID = &pCaptureInfos[deviceIndex].id;
            }
        }
    }

    // 3. Configure Capture Device
    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_capture);
    deviceConfig.capture.pDeviceID = pDeviceID;
    deviceConfig.capture.format    = ma_format_f32;
    deviceConfig.capture.channels  = 1;
    deviceConfig.sampleRate        = 44100;
    deviceConfig.dataCallback      = data_callback_record;
    deviceConfig.pUserData         = this;

    if (ma_device_init(&m_context, &deviceConfig, &m_captureDevice) != MA_SUCCESS) {
        logger->error("[SoundSystem] Failed to initialize capture device.");
        ma_encoder_uninit(&m_encoder);
        return false;
    }
    m_captureDeviceInitialized = true;

    if (ma_device_start(&m_captureDevice) != MA_SUCCESS) {
        std::cerr << "[SoundSystem] Failed to start capture device." << std::endl;
        ma_device_uninit(&m_captureDevice);
        ma_encoder_uninit(&m_encoder);
        m_captureDeviceInitialized = false;
        return false;
    }

    m_recordedFrames = 0;
    m_isRecording = true;
    return true;
}

void SoundSystem::stopRecording() {
    if (m_captureDeviceInitialized) {
        ma_device_uninit(&m_captureDevice);
        m_captureDeviceInitialized = false;
    }
    if (m_isRecording) {
        ma_encoder_uninit(&m_encoder);
        m_isRecording = false;
    }
}

bool SoundSystem::isRecording() const {
    return m_isRecording;
}

unsigned long long SoundSystem::get_recording_timestamp() {
    if (!m_isRecording || !m_captureDeviceInitialized) return 0;
    return static_cast<unsigned long long>((m_recordedFrames * 1000) / m_captureDevice.sampleRate);
}

void SoundSystem::data_callback_play(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    (void)pInput;
    SoundSystem* pSystem = (SoundSystem*)pDevice->pUserData;
    if (pSystem == NULL) return;

    ma_decoder_read_pcm_frames(&pSystem->m_decoder, pOutput, frameCount, NULL);
    pSystem->m_playedFrames += frameCount;
}

bool SoundSystem::play(const std::string& filePath) {
    if (m_isPlaying) stopPlaying();

    if (ma_decoder_init_file(filePath.c_str(), NULL, &m_decoder) != MA_SUCCESS) {
        logger->error("[SoundSystem] Could not load file: {}", filePath);
        return false;
    }

    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format   = m_decoder.outputFormat;
    deviceConfig.playback.channels = m_decoder.outputChannels;
    deviceConfig.sampleRate        = m_decoder.outputSampleRate;
    deviceConfig.dataCallback      = data_callback_play;
    deviceConfig.pUserData         = this;

    if (ma_device_init(&m_context, &deviceConfig, &m_playbackDevice) != MA_SUCCESS) {
        logger->error("[SoundSystem] Failed to open playback device.");
        ma_decoder_uninit(&m_decoder);
        return false;
    }
    m_playbackDeviceInitialized = true;

    if (ma_device_start(&m_playbackDevice) != MA_SUCCESS) {
        logger->error("[SoundSystem] Failed to start playback device.");
        ma_device_uninit(&m_playbackDevice);
        ma_decoder_uninit(&m_decoder);
        m_playbackDeviceInitialized = false;
        return false;
    }

    m_playedFrames = 0;
    m_isPlaying = true;
    return true;
}

void SoundSystem::stopPlaying() {
    if (m_playbackDeviceInitialized) {
        ma_device_uninit(&m_playbackDevice);
        m_playbackDeviceInitialized = false;
    }
    if (m_isPlaying) {
        ma_decoder_uninit(&m_decoder);
        m_isPlaying = false;
    }
}

bool SoundSystem::isPlaying() const {
    return m_isPlaying;
}

unsigned long long SoundSystem::get_playing_timestamp() {
    if (!m_isPlaying || !m_playbackDeviceInitialized) return 0;
    return static_cast<unsigned long long>((m_playedFrames * 1000) / m_playbackDevice.sampleRate);
}
