#include <iostream>
#include <vector>
#include <cstring>

#include "primitives/utils_logger.hpp"
#include "sound_system/sound_system.hpp"
#include "file_handler/sound_handler.hpp"
#include "configuration/configuration_system.hpp"

namespace {
    const uint32_t kOggCrcPoly = 0x04c11db7;
    const char kOpusHeadMagic[] = "OpusHead";
    const char kOpusTagsMagic[] = "OpusTags";
}

SoundSystem::SoundSystem(const SoundSystemConfig& config) 
    : m_config(config),
      m_sdlInitialized(false), 
      m_isRecording(false), 
      m_captureStream(nullptr),
      m_captureDeviceId(0),
      m_recordedFrames(0),
      m_isPlaying(false),
      m_playbackStream(nullptr),
      m_playbackDeviceId(0),
      m_playedFrames(0),
      m_opusEncoder(nullptr),
      m_recordFile(nullptr),
      m_opusDecoder(nullptr),
      m_playFile(nullptr)
{
    if (SDL_Init(SDL_INIT_AUDIO)) {
        m_sdlInitialized = true;
        logger->info("[SoundSystem] Initialized SDL Audio");
    } else {
        logger->error("[SoundSystem] Failed to initialize SDL Audio: {}", SDL_GetError());
    }
}

SoundSystem::~SoundSystem() {
    stopRecording();
    stopPlaying();
    if (m_sdlInitialized) {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
}

std::vector<SoundSystem::SoundSystemDeviceInfo> SoundSystem::getCaptureDevices() {
    std::vector<SoundSystemDeviceInfo> devices;
    if (!m_sdlInitialized) return devices;

    int count = 0;
    SDL_AudioDeviceID* deviceIds = SDL_GetAudioRecordingDevices(&count);
    if (deviceIds) {
        for (int i = 0; i < count; ++i) {
            const char* name = SDL_GetAudioDeviceName(deviceIds[i]);
            // SDL3 doesn't explicitly flag "default" in this list, but index 0 is usually default.
            // We store the ID as the index for simplicity in this DTO, or we could store the actual ID.
            // For compatibility with the interface expecting an int index, we'll use the loop index 
            // and re-fetch in startRecording, or we could cast the ID to int if it fits.
            // Here we stick to the index convention to map back later.
            devices.push_back({i, name ? std::string(name) : "Unknown Device", (i == 0)});
        }
        SDL_free(deviceIds);
    }
    return devices;
}

void SDLCALL SoundSystem::data_callback_record(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount) {
    SoundSystem* pSystem = (SoundSystem*)userdata;
    if (!pSystem) return;

    std::vector<float> tempBuffer(total_amount / sizeof(float));
    int bytesRead = SDL_GetAudioStreamData(stream, tempBuffer.data(), total_amount);
    int samplesRead = bytesRead / sizeof(float);

    pSystem->m_recordBuffer.insert(pSystem->m_recordBuffer.end(), tempBuffer.begin(), tempBuffer.begin() + samplesRead);
    
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
    pSystem->m_recordedFrames += samplesRead;

    unsigned int max_minutes = ConfigurationSystem::getInstance().get_max_recording_duration_minutes();
    if (max_minutes > 0) {
        uint64_t current_ms = (pSystem->m_recordedFrames * 1000ULL) / pSystem->m_config.sampleRate;
        uint64_t max_ms = static_cast<uint64_t>(max_minutes) * 60ULL * 1000ULL;
        if (current_ms >= max_ms) {
            logger->warn("[SoundSystem] Max recording duration limit of {} minutes reached. Stopping capture stream.", max_minutes);
            if (stream) {
                SDL_PauseAudioStreamDevice(stream);
            }
            pSystem->m_isRecording = false;
        }
    }
}

bool SoundSystem::startRecording(const SoundFileHandler& fileHandler, int deviceIndex) {
    if (!m_sdlInitialized) return false;
    if (m_isRecording) stopRecording();

    // 1. SoundSystemConfigure Encoder (Opus)
    int error;
    m_opusEncoder = opus_encoder_create(m_config.sampleRate, m_config.channels, OPUS_APPLICATION_AUDIO, &error);
    if (error != OPUS_OK) {
        logger->error("[SoundSystem] Failed to create Opus encoder.");
        return false;
    }
    std::string pathStr = fileHandler.get_file_path().string();
    m_recordFile = fopen(pathStr.c_str(), "wb");
    if (!m_recordFile) {
        logger->error("[SoundSystem] Failed to initialize output file: {}", pathStr);
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
    SDL_AudioDeviceID deviceID = SDL_AUDIO_DEVICE_DEFAULT_RECORDING;
    if (deviceIndex >= 0) {
        int count = 0;
        SDL_AudioDeviceID* deviceIds = SDL_GetAudioRecordingDevices(&count);
        if (deviceIds) {
            if (deviceIndex < count) {
                deviceID = deviceIds[deviceIndex];
            }
            SDL_free(deviceIds);
        }
    }

    // 3. Open Stream
    SDL_AudioSpec spec;
    SDL_zero(spec);
    spec.format = SDL_AUDIO_F32;
    spec.channels = m_config.channels;
    spec.freq = m_config.sampleRate;

    m_captureStream = SDL_OpenAudioDeviceStream(deviceID, &spec, data_callback_record, this);
    if (!m_captureStream) {
        logger->error("[SoundSystem] Failed to open capture stream: {}", SDL_GetError());
        opus_encoder_destroy(m_opusEncoder);
        m_opusEncoder = nullptr;
        fclose(m_recordFile);
        m_recordFile = nullptr;
        return false;
    }
    m_captureDeviceId = deviceID;

    // 4. Start Recording
    SDL_ResumeAudioStreamDevice(m_captureStream);

    m_recordedFrames = 0;
    m_isRecording = true;
    return true;
}

void SoundSystem::stopRecording() {
    if (m_captureStream) {
        // Flush remaining data? SDL_DestroyAudioStream handles cleanup.
        // We might want to pause first.
        SDL_PauseAudioStreamDevice(m_captureStream);
        SDL_DestroyAudioStream(m_captureStream);
        m_captureStream = nullptr;
        m_captureDeviceId = 0;
    }
    if (m_opusEncoder || m_recordFile || m_isRecording) {
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
    if (!m_isRecording) return 0;
    return static_cast<unsigned long long>((m_recordedFrames * 1000) / m_config.sampleRate);
}

void SDLCALL SoundSystem::data_callback_play(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount) {
    SoundSystem* pSystem = (SoundSystem*)userdata;
    if (!pSystem) return;

    const int frameSize = pSystem->m_config.frameSize;
    
    while (pSystem->m_playBuffer.size() * sizeof(float) < (size_t)additional_amount) {
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

    size_t bytesNeeded = additional_amount;
    size_t bytesAvailable = pSystem->m_playBuffer.size() * sizeof(float);
    size_t bytesToWrite = (bytesAvailable < bytesNeeded) ? bytesAvailable : bytesNeeded;

    if (bytesToWrite > 0) {
        SDL_PutAudioStreamData(stream, pSystem->m_playBuffer.data(), bytesToWrite);
        pSystem->m_playBuffer.erase(pSystem->m_playBuffer.begin(), pSystem->m_playBuffer.begin() + (bytesToWrite / sizeof(float)));
        pSystem->m_playedFrames += (bytesToWrite / sizeof(float));
    }

    if (pSystem->m_playbackCallback) {
        if (bytesToWrite < (size_t)additional_amount) {
            pSystem->m_playbackCallback(PlaybackEvent::PLAY_END, (unsigned int)pSystem->get_playing_timestamp());
        } else {
            pSystem->m_playbackCallback(PlaybackEvent::PLAY_TIMESTAMP, (unsigned int)pSystem->get_playing_timestamp());
        }
    }
}

bool SoundSystem::play(const SoundFileHandler& fileHandler, PlaybackCallback callback) {
    if (m_isPlaying) stopPlaying();

    m_playbackCallback = callback;

    std::uintmax_t duration = fileHandler.get_recorded_seconds();
    if (duration == 0) {
        duration = get_encrypted_file_duration(fileHandler.get_file_path().string());
    }

    std::error_code ec;
    auto fsize = std::filesystem::file_size(fileHandler.get_file_path(), ec);
    if (duration == 0 && (ec || fsize < 27)) {
        logger->error("[SoundSystem] Audio file has no duration. Playback aborted.");
        if (m_playbackCallback) m_playbackCallback(PlaybackEvent::PLAY_ERROR, 0);
        return false;
    }

    std::string pathStr = fileHandler.get_file_path().string();
    m_playFile = fopen(pathStr.c_str(), "rb");
    if (!m_playFile) {
        logger->error("[SoundSystem] Could not load file: {}", pathStr);
        if (m_playbackCallback) m_playbackCallback(PlaybackEvent::PLAY_ERROR, 0);
        return false;
    }
    int error;
    m_opusDecoder = opus_decoder_create(m_config.sampleRate, m_config.channels, &error);
    m_playBuffer.clear();

    SDL_AudioSpec spec;
    SDL_zero(spec);
    spec.format = SDL_AUDIO_F32;
    spec.channels = m_config.channels;
    spec.freq = m_config.sampleRate;

    m_playbackStream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, data_callback_play, this);
    if (!m_playbackStream) {
        logger->error("[SoundSystem] Failed to open playback stream: {}", SDL_GetError());
        opus_decoder_destroy(m_opusDecoder);
        m_opusDecoder = nullptr;
        fclose(m_playFile);
        m_playFile = nullptr;
        if (m_playbackCallback) m_playbackCallback(PlaybackEvent::PLAY_ERROR, 0);
        return false;
    }
    m_playbackDeviceId = SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;

    SDL_ResumeAudioStreamDevice(m_playbackStream);

    m_playedFrames = 0;
    m_isPlaying = true;
    if (m_playbackCallback) m_playbackCallback(PlaybackEvent::PLAY_START, 0);
    return true;
}

void SoundSystem::stopPlaying() {
    if (m_playbackStream) {
        SDL_DestroyAudioStream(m_playbackStream);
        m_playbackStream = nullptr;
        m_playbackDeviceId = 0;
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
        if (m_playbackCallback) {
            m_playbackCallback(PlaybackEvent::PLAY_STOP, (unsigned int)get_playing_timestamp());
            m_playbackCallback = nullptr;
        }
        m_isPlaying = false;
    }
}

bool SoundSystem::isPlaying() const {
    return m_isPlaying;
}

unsigned long long SoundSystem::get_playing_timestamp() {
    if (!m_isPlaying) return 0;
    return static_cast<unsigned long long>((m_playedFrames * 1000) / m_config.sampleRate);
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

std::uintmax_t SoundSystem::get_encrypted_file_duration(const std::string& filePath) {
    FILE* f = fopen(filePath.c_str(), "rb");
    if (!f) return 0;

    fseek(f, 0, SEEK_END);
    long fileSize = ftell(f);
    if (fileSize < 27) { // Minimum Ogg page size
        fclose(f);
        return 0;
    }

    // Scan the last 64KB (or the whole file if smaller) to find the last Ogg page
    long scanSize = (fileSize > 65536) ? 65536 : fileSize;
    long startPos = fileSize - scanSize;
    
    std::vector<uint8_t> buffer(scanSize);
    fseek(f, startPos, SEEK_SET);
    
    // secure_fread will decrypt the data in memory using the correct file offset
    if (secure_fread(buffer.data(), 1, scanSize, f) != (size_t)scanSize) {
        fclose(f);
        return 0;
    }
    fclose(f);

    // Scan backwards for the "OggS" capture pattern
    for (long i = scanSize - 4; i >= 0; --i) {
        if (buffer[i] == 'O' && buffer[i+1] == 'g' && buffer[i+2] == 'g' && buffer[i+3] == 'S') {
            // Found OggS. The granule position is at offset 6 (8 bytes, little-endian)
            if (i + 14 <= scanSize) {
                uint64_t granulePos = 0;
                for (int j = 0; j < 8; ++j) {
                    granulePos |= (uint64_t)buffer[i + 6 + j] << (j * 8);
                }
                // Opus always uses 48kHz for granule position
                return (std::uintmax_t)(granulePos / 48000);
            }
        }
    }
    return 0;
}

rst_code_e SoundSystem::read_decrypted_audio_range(
    const SoundFileHandler& fileHandler,
    uint64_t offset,
    size_t length,
    std::vector<uint8_t>& out_buffer,
    bool& out_is_eof
) {
    out_buffer.clear();
    out_is_eof = false;

    std::string pathStr = fileHandler.get_file_path().string();
    if (pathStr.empty()) {
        logger->error("[SoundSystem] File path is empty");
        return FILE_NOT_FOUND;
    }

    FILE* f = fopen(pathStr.c_str(), "rb");
    if (!f) {
        logger->error("[SoundSystem] Could not open file: {}", pathStr);
        return FILE_NOT_FOUND;
    }

    fseek(f, 0, SEEK_END);
    long fileSize = ftell(f);
    if (fileSize < 0) {
        fclose(f);
        logger->error("[SoundSystem] Failed to determine file size for: {}", pathStr);
        return FILE_READ_ERROR;
    }

    if (offset >= static_cast<uint64_t>(fileSize)) {
        fclose(f);
        out_is_eof = true;
        return RST_OK;
    }

    if (fseek(f, static_cast<long>(offset), SEEK_SET) != 0) {
        fclose(f);
        logger->error("[SoundSystem] Failed to seek to offset {} in: {}", offset, pathStr);
        return FILE_READ_ERROR;
    }

    size_t bytesToRead = length;
    if (offset + bytesToRead >= static_cast<uint64_t>(fileSize)) {
        bytesToRead = static_cast<size_t>(static_cast<uint64_t>(fileSize) - offset);
        out_is_eof = true;
    }

    if (bytesToRead == 0) {
        fclose(f);
        return RST_OK;
    }

    out_buffer.resize(bytesToRead);
    size_t readCount = secure_fread(out_buffer.data(), 1, bytesToRead, f);
    fclose(f);

    if (readCount < bytesToRead) {
        out_buffer.resize(readCount);
        out_is_eof = true;
    }

    return RST_OK;
}

