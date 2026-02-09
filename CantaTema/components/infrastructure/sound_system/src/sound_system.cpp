#include <iostream>
#include <vector>
#include <cstring>

#include "primitives/utils_logger.hpp"
#include "sound_system/sound_system.hpp"

namespace {
    const uint32_t kOggCrcPoly = 0x04c11db7;
    const char kOpusHeadMagic[] = "OpusHead";
    const char kOpusTagsMagic[] = "OpusTags";
}

SoundSystem::SoundSystem(const SoundSystemConfig& config) 
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

std::vector<SoundSystem::SoundSystemDeviceInfo> SoundSystem::getCaptureDevices() {
    std::vector<SoundSystemDeviceInfo> devices;
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
            // Update granule position (48kHz)
            // Note: Opus frame size is usually 48000 * 0.020 = 960 samples
            // We assume input is 48k or resampled by miniaudio, but here we just add frameSize
            // because opus_encode_float consumes frameSize samples.
            pSystem->m_oggState.granulePos += frameSize; 
            
            // Write Ogg Page (Simple: 1 packet per page)
            pSystem->write_ogg_page(pSystem->m_recordFile, pSystem->m_oggState, encodedData, len, 0);
        }
        pSystem->m_recordBuffer.erase(pSystem->m_recordBuffer.begin(), pSystem->m_recordBuffer.begin() + frameSize);
    }
    pSystem->m_recordedFrames += frameCount;
}

bool SoundSystem::startRecording(const std::string& filePath, int deviceIndex) {
    if (!m_contextInitialized) return false;
    if (m_isRecording) stopRecording();

    // 1. SoundSystemConfigure Encoder (Opus)
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

    // Initialize Ogg State
    {
        m_oggState.serial = (uint32_t)rand();
        m_oggState.sequence = 0;
        m_oggState.granulePos = 0;

        // 1. Write ID Header (OpusHead)
        std::vector<uint8_t> opusHead;
        opusHead.insert(opusHead.end(), kOpusHeadMagic, kOpusHeadMagic + 8);
        opusHead.push_back(1); // Version
        opusHead.push_back(m_config.channels); // Channels
        
        // Pre-skip (Lookahead). Important for sync.
        int32_t lookahead = 0;
        opus_encoder_ctl(m_opusEncoder, OPUS_GET_LOOKAHEAD(&lookahead));
        write_le16(opusHead, (uint16_t)lookahead);
        
        write_le32(opusHead, m_config.sampleRate); // Input Sample Rate
        write_le16(opusHead, 0); // Gain
        opusHead.push_back(0); // Channel Mapping Family

        write_ogg_page(m_recordFile, m_oggState, opusHead.data(), opusHead.size(), 0x02); // BOS

        // 2. Write Comment Header (OpusTags)
        std::vector<uint8_t> opusTags;
        opusTags.insert(opusTags.end(), kOpusTagsMagic, kOpusTagsMagic + 8);
        
        std::string vendor = "CantaTema";
        write_le32(opusTags, vendor.length());
        opusTags.insert(opusTags.end(), vendor.begin(), vendor.end());
        
        write_le32(opusTags, 0); // User Comment List Length

        write_ogg_page(m_recordFile, m_oggState, opusTags.data(), opusTags.size(), 0);
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

    // 3. SoundSystemConfigure Capture Device
    ma_device_config deviceSoundSystemConfig = ma_device_config_init(ma_device_type_capture);
    deviceSoundSystemConfig.capture.pDeviceID = pDeviceID;
    deviceSoundSystemConfig.capture.format    = ma_format_f32;
    deviceSoundSystemConfig.capture.channels  = m_config.channels;
    deviceSoundSystemConfig.sampleRate        = m_config.sampleRate;
    deviceSoundSystemConfig.dataCallback      = data_callback_record;
    deviceSoundSystemConfig.pUserData         = this;

    if (ma_device_init(&m_context, &deviceSoundSystemConfig, &m_captureDevice) != MA_SUCCESS) {
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
            // Write EOS Page
            write_ogg_page(m_recordFile, m_oggState, nullptr, 0, 0x04); // EOS
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
        // Simple Ogg Page Parser
        // 1. Find OggS
        char capture[4];
        if (pSystem->secure_fread(capture, 1, 4, pSystem->m_playFile) != 4) break;
        if (memcmp(capture, "OggS", 4) != 0) {
            // Not Ogg or lost sync, try to skip byte by byte? 
            // For simplicity, we assume valid file or stop.
            break; 
        }

        // 2. Read rest of header (23 bytes)
        uint8_t header[23];
        if (pSystem->secure_fread(header, 1, 23, pSystem->m_playFile) != 23) break;

        int segments = header[22]; // Byte 26 in file is byte 22 here
        std::vector<uint8_t> segmentTable(segments);
        if (pSystem->secure_fread(segmentTable.data(), 1, segments, pSystem->m_playFile) != (size_t)segments) break;

        // 3. Reconstruct Packet
        // Note: This simple parser assumes 1 packet per page or handles lacing simply
        std::vector<uint8_t> packetData;
        for (int s : segmentTable) {
            std::vector<uint8_t> segment(s);
            if (pSystem->secure_fread(segment.data(), 1, s, pSystem->m_playFile) != (size_t)s) break;
            packetData.insert(packetData.end(), segment.begin(), segment.end());
        }

        // Check if it is a header packet (OpusHead/OpusTags) and skip decoding
        if (packetData.size() >= 8) {
            if (memcmp(packetData.data(), "OpusHead", 8) == 0 || 
                memcmp(packetData.data(), "OpusTags", 8) == 0) {
                continue; // Skip headers
            }
        }

        if (packetData.empty()) continue; // Empty page (e.g. EOS)
        
        // 4. Decode
        // Note: Standard Opus files might have multiple packets per page.
        // This parser treats the whole page payload as one packet (valid for our writer).
        // A full Ogg reader would iterate through lacing values < 255.
        // For robustness with standard files, we should handle lacing, but this is a minimal implementation.

        std::vector<float> decodedFrame(frameSize);
        int decodedSamples = opus_decode_float(pSystem->m_opusDecoder, packetData.data(), packetData.size(), decodedFrame.data(), frameSize, 0);
        
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

    ma_device_config deviceSoundSystemConfig = ma_device_config_init(ma_device_type_playback);
    deviceSoundSystemConfig.playback.format   = ma_format_f32;
    deviceSoundSystemConfig.playback.channels = m_config.channels;
    deviceSoundSystemConfig.sampleRate        = m_config.sampleRate;
    deviceSoundSystemConfig.dataCallback      = data_callback_play;
    deviceSoundSystemConfig.pUserData         = this;

    if (ma_device_init(&m_context, &deviceSoundSystemConfig, &m_playbackDevice) != MA_SUCCESS) {
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

// --------------------------------------------------------------------------------------
// Private Helper Implementations
// --------------------------------------------------------------------------------------

uint32_t SoundSystem::update_crc(uint32_t crc, const uint8_t* data, size_t len) {
    static bool table_initialized = false;
    static uint32_t crc_table[256];

    if (!table_initialized) {
        for (int i = 0; i < 256; i++) {
            uint32_t r = i << 24;
            for (int j = 0; j < 8; j++) {
                if (r & 0x80000000) r = (r << 1) ^ kOggCrcPoly;
                else r <<= 1;
            }
            crc_table[i] = r;
        }
        table_initialized = true;
    }

    for (size_t i = 0; i < len; i++) {
        crc = (crc << 8) ^ crc_table[((crc >> 24) & 0xFF) ^ data[i]];
    }
    return crc;
}

void SoundSystem::write_le32(std::vector<uint8_t>& buf, uint32_t val) {
    buf.push_back(val & 0xFF);
    buf.push_back((val >> 8) & 0xFF);
    buf.push_back((val >> 16) & 0xFF);
    buf.push_back((val >> 24) & 0xFF);
}

void SoundSystem::write_le64(std::vector<uint8_t>& buf, uint64_t val) {
    write_le32(buf, val & 0xFFFFFFFF);
    write_le32(buf, val >> 32);
}

void SoundSystem::write_le16(std::vector<uint8_t>& buf, uint16_t val) {
    buf.push_back(val & 0xFF);
    buf.push_back((val >> 8) & 0xFF);
}

void SoundSystem::xor_process(void* data, size_t len, long offset) {
    if (m_config.encryptionKey.empty()) return;
    
    uint8_t* bytes = static_cast<uint8_t*>(data);
    size_t keyLen = m_config.encryptionKey.length();
    for (size_t i = 0; i < len; ++i) {
        bytes[i] ^= m_config.encryptionKey[(offset + i) % keyLen];
    }
}

size_t SoundSystem::secure_fwrite(const void* ptr, size_t size, size_t count, FILE* stream) {
    if (m_config.encryptionKey.empty()) {
        return fwrite(ptr, size, count, stream);
    }
    
    long pos = ftell(stream);
    size_t totalBytes = size * count;
    if (totalBytes == 0) return 0;
    
    std::vector<uint8_t> temp(totalBytes);
    std::memcpy(temp.data(), ptr, totalBytes);
    xor_process(temp.data(), totalBytes, pos);
    
    return fwrite(temp.data(), 1, totalBytes, stream) / size;
}

size_t SoundSystem::secure_fread(void* ptr, size_t size, size_t count, FILE* stream) {
    if (m_config.encryptionKey.empty()) {
        return fread(ptr, size, count, stream);
    }

    long pos = ftell(stream);
    size_t readCount = fread(ptr, size, count, stream);
    if (readCount > 0) {
        xor_process(ptr, readCount * size, pos);
    }
    return readCount;
}

void SoundSystem::write_ogg_page(FILE* file, OggStreamState& state, const uint8_t* packet, int packetLen, int headerType) {
    std::vector<uint8_t> page;
    page.push_back('O'); page.push_back('g'); page.push_back('g'); page.push_back('S');
    page.push_back(0);
    page.push_back(headerType);
    write_le64(page, state.granulePos);
    write_le32(page, state.serial);
    write_le32(page, state.sequence++);
    write_le32(page, 0); // Checksum placeholder
    int segments = (packetLen + 255) / 255;
    page.push_back(segments);
    int remaining = packetLen;
    for (int i = 0; i < segments; i++) {
        int size = (remaining > 255) ? 255 : remaining;
        page.push_back(size);
        remaining -= size;
    }
    uint32_t crc = 0;
    crc = update_crc(crc, page.data(), page.size());
    if (packet && packetLen > 0) {
        crc = update_crc(crc, packet, packetLen);
    }
    page[22] = crc & 0xFF;
    page[23] = (crc >> 8) & 0xFF;
    page[24] = (crc >> 16) & 0xFF;
    page[25] = (crc >> 24) & 0xFF;
    secure_fwrite(page.data(), 1, page.size(), file);
    if (packet && packetLen > 0) {
        secure_fwrite(packet, 1, packetLen, file);
    }
}
