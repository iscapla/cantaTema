#include <iostream>

#include "primitives/utils_logger.hpp"
#include "sound_system/sound_system.hpp"


SoundSystem::SoundSystem(const Config& config) 
    : m_config(config),
      m_contextInitialized(false), 
      m_isRecording(false), 
      m_captureDeviceInitialized(false),
      m_recordedFrames(0),
      m_isPlaying(false),
      m_playbackDeviceInitialized(false),
      m_playedFrames(0),
      m_opusEncoder(nullptr),
      m_recordFile(nullptr),
      m_opusDecoder(nullptr),
      m_playFile(nullptr)
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

    const float* inputFloats = (const float*)pInput;
    pSystem->m_recordBuffer.insert(pSystem->m_recordBuffer.end(), inputFloats, inputFloats + frameCount);

    const int frameSize = pSystem->m_config.frameSize;
    unsigned char encodedData[4000];

    while (pSystem->m_recordBuffer.size() >= frameSize) {
        int len = opus_encode_float(pSystem->m_opusEncoder, pSystem->m_recordBuffer.data(), frameSize, encodedData, sizeof(encodedData));
        if (len > 0) {
            fwrite(&len, sizeof(int), 1, pSystem->m_recordFile);
            fwrite(encodedData, 1, len, pSystem->m_recordFile);
        }
        pSystem->m_recordBuffer.erase(pSystem->m_recordBuffer.begin(), pSystem->m_recordBuffer.begin() + frameSize);
    }
    pSystem->m_recordedFrames += frameCount;
}

bool SoundSystem::startRecording(const std::string& filePath, int deviceIndex) {
    if (!m_contextInitialized) return false;
    if (m_isRecording) stopRecording();

    // 1. Configure Encoder (Opus)
    int error;
    m_opusEncoder = opus_encoder_create(m_config.sampleRate, m_config.channels, OPUS_APPLICATION_AUDIO, &error);
    if (error != OPUS_OK) {
        logger->error("[SoundSystem] Failed to create Opus encoder.");
        return false;
    }
    m_recordFile = fopen(filePath.c_str(), "wb");
    if (!m_recordFile) {
        logger->error("[SoundSystem] Failed to initialize output file: {}", filePath);
        opus_encoder_destroy(m_opusEncoder);
        m_opusEncoder = nullptr;
        return false;
    }
    m_recordBuffer.clear();

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
    deviceConfig.capture.channels  = m_config.channels;
    deviceConfig.sampleRate        = m_config.sampleRate;
    deviceConfig.dataCallback      = data_callback_record;
    deviceConfig.pUserData         = this;

    if (ma_device_init(&m_context, &deviceConfig, &m_captureDevice) != MA_SUCCESS) {
        logger->error("[SoundSystem] Failed to initialize capture device.");
        opus_encoder_destroy(m_opusEncoder);
        m_opusEncoder = nullptr;
        fclose(m_recordFile);
        m_recordFile = nullptr;
        return false;
    }
    m_captureDeviceInitialized = true;

    if (ma_device_start(&m_captureDevice) != MA_SUCCESS) {
        std::cerr << "[SoundSystem] Failed to start capture device." << std::endl;
        ma_device_uninit(&m_captureDevice);
        opus_encoder_destroy(m_opusEncoder);
        m_opusEncoder = nullptr;
        fclose(m_recordFile);
        m_recordFile = nullptr;
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
        if (m_opusEncoder) {
            opus_encoder_destroy(m_opusEncoder);
            m_opusEncoder = nullptr;
        }
        if (m_recordFile) {
            fclose(m_recordFile);
            m_recordFile = nullptr;
        }
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

    float* outputFloats = (float*)pOutput;
    const int frameSize = pSystem->m_config.frameSize;
    
    while (pSystem->m_playBuffer.size() < frameCount) {
        int len = 0;
        if (fread(&len, sizeof(int), 1, pSystem->m_playFile) != 1) break;
        
        std::vector<unsigned char> encodedData(len);
        if (fread(encodedData.data(), 1, len, pSystem->m_playFile) != (size_t)len) break;

        std::vector<float> decodedFrame(frameSize);
        int decodedSamples = opus_decode_float(pSystem->m_opusDecoder, encodedData.data(), len, decodedFrame.data(), frameSize, 0);
        
        if (decodedSamples > 0) {
            pSystem->m_playBuffer.insert(pSystem->m_playBuffer.end(), decodedFrame.begin(), decodedFrame.begin() + decodedSamples);
        }
    }

    size_t available = pSystem->m_playBuffer.size();
    size_t toCopy = (available < frameCount) ? available : frameCount;
    
    for (size_t i = 0; i < toCopy; ++i) {
        outputFloats[i] = pSystem->m_playBuffer[i];
    }
    for (size_t i = toCopy; i < frameCount; ++i) {
        outputFloats[i] = 0.0f;
    }

    if (toCopy > 0) {
        pSystem->m_playBuffer.erase(pSystem->m_playBuffer.begin(), pSystem->m_playBuffer.begin() + toCopy);
    }
    pSystem->m_playedFrames += frameCount;
}

bool SoundSystem::play(const std::string& filePath) {
    if (m_isPlaying) stopPlaying();

    m_playFile = fopen(filePath.c_str(), "rb");
    if (!m_playFile) {
        logger->error("[SoundSystem] Could not load file: {}", filePath);
        return false;
    }
    int error;
    m_opusDecoder = opus_decoder_create(m_config.sampleRate, m_config.channels, &error);
    m_playBuffer.clear();

    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format   = ma_format_f32;
    deviceConfig.playback.channels = m_config.channels;
    deviceConfig.sampleRate        = m_config.sampleRate;
    deviceConfig.dataCallback      = data_callback_play;
    deviceConfig.pUserData         = this;

    if (ma_device_init(&m_context, &deviceConfig, &m_playbackDevice) != MA_SUCCESS) {
        logger->error("[SoundSystem] Failed to open playback device.");
        opus_decoder_destroy(m_opusDecoder);
        m_opusDecoder = nullptr;
        fclose(m_playFile);
        m_playFile = nullptr;
        return false;
    }
    m_playbackDeviceInitialized = true;

    if (ma_device_start(&m_playbackDevice) != MA_SUCCESS) {
        logger->error("[SoundSystem] Failed to start playback device.");
        ma_device_uninit(&m_playbackDevice);
        opus_decoder_destroy(m_opusDecoder);
        m_opusDecoder = nullptr;
        fclose(m_playFile);
        m_playFile = nullptr;
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
        if (m_opusDecoder) {
            opus_decoder_destroy(m_opusDecoder);
            m_opusDecoder = nullptr;
        }
        if (m_playFile) {
            fclose(m_playFile);
            m_playFile = nullptr;
        }
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
