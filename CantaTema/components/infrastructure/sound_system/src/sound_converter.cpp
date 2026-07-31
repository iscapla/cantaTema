#include "sound_system/sound_converter.hpp"

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>

#include "opus.h"
#include "primitives/utils_logger.hpp"

bool SoundConverter::convert_opus_to_wav(const std::string& opus_path, const std::string& wav_out_path) {
    FILE* f = fopen(opus_path.c_str(), "rb");
    if (!f) {
        if (logger) logger->error("[SoundConverter] Failed to open input Opus file: {}", opus_path);
        return false;
    }

    char magic[4];
    if (fread(magic, 1, 4, f) != 4) {
        if (logger) logger->error("[SoundConverter] Failed to read magic bytes from: {}", opus_path);
        fclose(f);
        return false;
    }
    fseek(f, 0, SEEK_SET);

    std::string encryptionKey = "";
    if (memcmp(magic, "OggS", 4) != 0) {
        std::string testKey = "CantaTemaSecretKey";
        char testBuf[4];
        std::memcpy(testBuf, magic, 4);
        for (int i = 0; i < 4; ++i) {
            testBuf[i] ^= testKey[i % testKey.length()];
        }
        if (memcmp(testBuf, "OggS", 4) == 0) {
            encryptionKey = testKey;
        }
    }

    auto secure_read = [&](void* ptr, size_t size, size_t count) -> size_t {
        if (encryptionKey.empty()) {
            return fread(ptr, size, count, f);
        }
        long pos = ftell(f);
        size_t readCount = fread(ptr, size, count, f);
        if (readCount > 0) {
            uint8_t* bytes = static_cast<uint8_t*>(ptr);
            size_t keyLen = encryptionKey.length();
            for (size_t i = 0; i < readCount * size; ++i) {
                bytes[i] ^= encryptionKey[(pos + i) % keyLen];
            }
        }
        return readCount;
    };

    int err = 0;
    int sampleRate = 48000;
    int channels = 1;
    OpusDecoder* decoder = opus_decoder_create(sampleRate, channels, &err);
    if (!decoder || err != OPUS_OK) {
        if (logger) logger->error("[SoundConverter] Failed to create Opus decoder (err code: {})", err);
        if (decoder) opus_decoder_destroy(decoder);
        fclose(f);
        return false;
    }

    std::vector<int16_t> all_pcm_samples;
    const int frameSize = 960; // 20ms at 48kHz

    while (true) {
        char capture[4];
        if (secure_read(capture, 1, 4) != 4) break;
        if (memcmp(capture, "OggS", 4) != 0) break;

        uint8_t header[23];
        if (secure_read(header, 1, 23) != 23) break;

        int segments = header[22];
        std::vector<uint8_t> segmentTable(segments);
        if (secure_read(segmentTable.data(), 1, segments) != static_cast<size_t>(segments)) break;

        std::vector<uint8_t> packetData;
        for (int s : segmentTable) {
            std::vector<uint8_t> segment(s);
            if (secure_read(segment.data(), 1, s) != static_cast<size_t>(s)) break;
            packetData.insert(packetData.end(), segment.begin(), segment.end());
        }

        if (packetData.size() >= 8) {
            if (memcmp(packetData.data(), "OpusHead", 8) == 0 ||
                memcmp(packetData.data(), "OpusTags", 8) == 0) {
                continue;
            }
        }

        if (packetData.empty()) continue;

        std::vector<int16_t> decodedFrame(frameSize * channels);
        int decodedSamples = opus_decode(decoder, packetData.data(), static_cast<opus_int32>(packetData.size()), decodedFrame.data(), frameSize, 0);
        if (decodedSamples > 0) {
            all_pcm_samples.insert(all_pcm_samples.end(), decodedFrame.begin(), decodedFrame.begin() + (decodedSamples * channels));
        }
    }

    opus_decoder_destroy(decoder);
    fclose(f);

    if (all_pcm_samples.empty()) {
        if (logger) logger->error("[SoundConverter] No PCM samples decoded from Opus file: {}", opus_path);
        return false;
    }

    FILE* out_wav = fopen(wav_out_path.c_str(), "wb");
    if (!out_wav) {
        if (logger) logger->error("[SoundConverter] Failed to open output WAV file for writing: {}", wav_out_path);
        return false;
    }

    uint32_t dataSize = static_cast<uint32_t>(all_pcm_samples.size() * sizeof(int16_t));
    uint32_t chunkSize = 36 + dataSize;
    uint32_t subchunk1Size = 16;
    uint16_t audioFormat = 1;
    uint16_t numChannels = static_cast<uint16_t>(channels);
    uint32_t wavSampleRate = static_cast<uint32_t>(sampleRate);
    uint16_t bitsPerSample = 16;
    uint32_t byteRate = wavSampleRate * numChannels * (bitsPerSample / 8);
    uint16_t blockAlign = numChannels * (bitsPerSample / 8);

    fwrite("RIFF", 1, 4, out_wav);
    fwrite(&chunkSize, 4, 1, out_wav);
    fwrite("WAVE", 1, 4, out_wav);
    fwrite("fmt ", 1, 4, out_wav);
    fwrite(&subchunk1Size, 4, 1, out_wav);
    fwrite(&audioFormat, 2, 1, out_wav);
    fwrite(&numChannels, 2, 1, out_wav);
    fwrite(&wavSampleRate, 4, 1, out_wav);
    fwrite(&byteRate, 4, 1, out_wav);
    fwrite(&blockAlign, 2, 1, out_wav);
    fwrite(&bitsPerSample, 2, 1, out_wav);
    fwrite("data", 1, 4, out_wav);
    fwrite(&dataSize, 4, 1, out_wav);
    fwrite(all_pcm_samples.data(), sizeof(int16_t), all_pcm_samples.size(), out_wav);
    fclose(out_wav);

    if (logger) logger->info("[SoundConverter] Successfully converted OPUS ('{}') to WAV ('{}') - {} samples",
                             opus_path, wav_out_path, all_pcm_samples.size());
    return true;
}
