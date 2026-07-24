#include "speech_recognition/whisper_speech_recognition.hpp"

#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <chrono>
#include <cstring>
#include <memory>

#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>

#include "whisper.h"
#include "opus.h"
#include "speech_recognition/gpu_detector.hpp"
#include "primitives/tool_paths.hpp"
#include "primitives/utils_logger.hpp"
#include "file_handler/sound_handler.hpp"

namespace {

/**
 * @brief RAII guard that automatically removes a temporary file on scope exit
 *        unless explicitly told to keep it. Ensures no temp files remain if an error occurs.
 */
struct TempFileGuard {
    std::filesystem::path path;
    bool keep = false;

    explicit TempFileGuard(const std::filesystem::path& p) : path(p) {}
    ~TempFileGuard() {
        if (!keep && !path.empty()) {
            std::error_code ec;
            if (std::filesystem::exists(path, ec)) {
                std::filesystem::remove(path, ec);
            }
        }
    }

    TempFileGuard(const TempFileGuard&) = delete;
    TempFileGuard& operator=(const TempFileGuard&) = delete;
};

/**
 * @brief Decode an Ogg/Opus audio file (encrypted or unencrypted) and write it out as a WAV file.
 */
static bool convert_opus_to_wav(const std::string& opus_path, const std::string& wav_out_path) {
    FILE* f = fopen(opus_path.c_str(), "rb");
    if (!f) return false;

    char magic[4];
    if (fread(magic, 1, 4, f) != 4) {
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
        return false;
    }

    FILE* out_wav = fopen(wav_out_path.c_str(), "wb");
    if (!out_wav) return false;

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

    return true;
}

} // anonymous namespace

WhisperSpeechRecognition::WhisperSpeechRecognition(std::shared_ptr<ISoundSystem> sound_system)
    : m_soundSystem(sound_system) {}

WhisperSpeechRecognition::~WhisperSpeechRecognition() {
    if (m_whisperCtx) {
        whisper_free(m_whisperCtx);
        m_whisperCtx = nullptr;
    }
}

rst_code_e WhisperSpeechRecognition::initialize(const speech_recognition_config_t& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config = config;
    m_status = speech_recognition_status_e::IDLE;

    if (m_whisperCtx) {
        whisper_free(m_whisperCtx);
        m_whisperCtx = nullptr;
    }

    std::filesystem::path model_path;
    if (std::filesystem::exists(config.model_name)) {
        model_path = config.model_name;
    } else {
        model_path = ToolPath::get_path_for_models_whisper() / config.model_name;
        if (!std::filesystem::exists(model_path)) {
            std::filesystem::path alt_path = ToolPath::get_path_for_models_whisper() / ("ggml-" + config.model_name + ".bin");
            if (std::filesystem::exists(alt_path)) {
                model_path = alt_path;
            }
        }
    }

    if (!std::filesystem::exists(model_path)) {
        if (logger) logger->error("[WhisperSpeechRecognition] Whisper model file not found at: {}", model_path.string());
        m_status = speech_recognition_status_e::ERROR;
        return FILE_NOT_FOUND;
    }

    cantatema::infra::AccelerationReport accel_report = cantatema::infra::detect_accelerators();

    struct whisper_context_params cparams = whisper_context_default_params();
    cparams.use_gpu = config.use_gpu || accel_report.use_gpu;
    cparams.flash_attn = false;

    if (logger) logger->info("[WhisperSpeechRecognition] Initializing whisper context (GPU acceleration strategy: {}, use_gpu: {})",
                             accel_report.selected_device_name, cparams.use_gpu ? "ON" : "OFF");

    m_whisperCtx = whisper_init_from_file_with_params(model_path.string().c_str(), cparams);
    if (!m_whisperCtx && cparams.use_gpu) {
        if (logger) logger->warn("[WhisperSpeechRecognition] GPU context initialization failed or no supported GPU found. Retrying with CPU mode...");
        cparams.use_gpu = false;
        m_whisperCtx = whisper_init_from_file_with_params(model_path.string().c_str(), cparams);
    }

    if (!m_whisperCtx) {
        if (logger) logger->error("[WhisperSpeechRecognition] Failed to initialize whisper context from: {}", model_path.string());
        m_status = speech_recognition_status_e::ERROR;
        return UNKNOWN;
    }

    if (logger) logger->info("[WhisperSpeechRecognition] Whisper context initialized with model: {}", model_path.string());
    return RST_OK;
}

std::vector<float> WhisperSpeechRecognition::decode_audio_to_pcm(const std::string& audio_file_path) {
    std::filesystem::path input_path(audio_file_path);
    std::string file_to_load = audio_file_path;
    std::unique_ptr<TempFileGuard> temp_guard;

    // Convert Opus input to temporary WAV file if necessary
    if (input_path.extension() == ".opus") {
        std::filesystem::path temp_wav = std::filesystem::temp_directory_path() /
            ("cantatema_temp_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".wav");

        temp_guard = std::make_unique<TempFileGuard>(temp_wav);

        if (logger) logger->info("[WhisperSpeechRecognition] Converting Opus file ('{}') to temporary WAV ('{}')...",
                                 audio_file_path, temp_wav.string());

        if (!convert_opus_to_wav(audio_file_path, temp_wav.string())) {
            if (logger) logger->error("[WhisperSpeechRecognition] Failed to convert Opus file to WAV: {}", audio_file_path);
            return {}; // temp_guard automatically cleans up temp WAV file if created
        }
        file_to_load = temp_wav.string();
    }

    if (!SDL_Init(SDL_INIT_AUDIO)) {
        if (logger) logger->error("SDL_Init(SDL_INIT_AUDIO) failed: {}", SDL_GetError());
        return {}; // temp_guard automatically cleans up
    }

    SDL_AudioSpec src_spec{};
    Uint8* src_data = nullptr;
    Uint32 src_len = 0;

    if (!SDL_LoadWAV(file_to_load.c_str(), &src_spec, &src_data, &src_len)) {
        if (logger) logger->error("SDL_LoadWAV('{}') failed: {}", file_to_load, SDL_GetError());
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return {}; // temp_guard automatically cleans up
    }

    if (logger) logger->debug("WAV source — format: 0x{:04X}, channels: {}, freq: {} Hz, bytes: {}",
                              static_cast<unsigned>(src_spec.format), src_spec.channels,
                              src_spec.freq, src_len);

    SDL_AudioSpec dst_spec{};
    dst_spec.format = SDL_AUDIO_F32;
    dst_spec.channels = 1;
    dst_spec.freq = WHISPER_SAMPLE_RATE;

    Uint8* dst_data = nullptr;
    int dst_len = 0;

    if (!SDL_ConvertAudioSamples(&src_spec, src_data, static_cast<int>(src_len),
                                  &dst_spec, &dst_data, &dst_len)) {
        if (logger) logger->error("SDL_ConvertAudioSamples failed: {}", SDL_GetError());
        SDL_free(src_data);
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return {}; // temp_guard automatically cleans up
    }

    SDL_free(src_data);

    const std::size_t n_samples = static_cast<std::size_t>(dst_len) / sizeof(float);
    const auto* fptr = reinterpret_cast<const float*>(dst_data);
    std::vector<float> pcmf32(fptr, fptr + n_samples);

    SDL_free(dst_data);
    SDL_QuitSubSystem(SDL_INIT_AUDIO);

    // temp_guard goes out of scope here and automatically deletes the temporary WAV file
    return pcmf32;
}

rst_code_e WhisperSpeechRecognition::submit_task(const std::string& audio_file_path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_whisperCtx) {
        if (logger) logger->error("[WhisperSpeechRecognition] submit_task failed: whisper context not initialized.");
        m_status = speech_recognition_status_e::ERROR;
        return UNKNOWN;
    }

    if (!std::filesystem::exists(audio_file_path)) {
        if (logger) logger->error("[WhisperSpeechRecognition] Audio file not found: {}", audio_file_path);
        m_status = speech_recognition_status_e::ERROR;
        return FILE_NOT_FOUND;
    }

    m_status = speech_recognition_status_e::PROCESSING;
    if (m_config.status_callback) {
        m_config.status_callback(m_status);
    }

    m_segments.clear();

    struct whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);

    std::string lang = m_config.language.empty() ? "es" : m_config.language;
    wparams.language        = lang.c_str();
    wparams.n_threads       = 8; // Optimal P-core threads
    wparams.duration_ms     = 0; // 0 = process full audio file
    wparams.print_progress  = false;
    wparams.print_special   = false;
    wparams.print_realtime  = false;
    wparams.print_timestamps = false;
    wparams.no_timestamps   = false;
    wparams.single_segment  = false;
    wparams.translate       = false;
    wparams.token_timestamps = true;

    wparams.temperature_inc = 0.0f;  // Disable fallback retries on low confidence/silence
    wparams.greedy.best_of  = 1;     // Fast single-pass greedy decoding
    wparams.no_context      = true;  // Disable past text context overhead

    wparams.new_segment_callback = [](struct whisper_context* ctx_cb, struct whisper_state*, int n_new, void*) {
        const int n_segments = whisper_full_n_segments(ctx_cb);
        for (int i = n_segments - n_new; i < n_segments; ++i) {
            const char* text = whisper_full_get_segment_text(ctx_cb, i);
            const int64_t t0 = whisper_full_get_segment_t0(ctx_cb, i) * 10;
            const int64_t t1 = whisper_full_get_segment_t1(ctx_cb, i) * 10;
            const double s0  = static_cast<double>(t0) / 1000.0;
            const double s1  = static_cast<double>(t1) / 1000.0;

            if (logger) logger->debug("[seg {:02d}] [{:.2f}s -> {:.2f}s] {}", i, s0, s1, text ? text : "");
        }
    };

    wparams.progress_callback = [](struct whisper_context*, struct whisper_state*, int progress, void*) {
        static int last_reported_progress = -1;
        if (progress != last_reported_progress) {
            if (logger) logger->info("Transcription progress: {}%", progress);
            last_reported_progress = progress;
        }
    };

    std::vector<float> pcm_samples = decode_audio_to_pcm(audio_file_path);
    if (pcm_samples.empty()) {
        if (logger) logger->error("[WhisperSpeechRecognition] Failed to decode audio from: {}", audio_file_path);
        m_status = speech_recognition_status_e::ERROR;
        if (m_config.status_callback) {
            m_config.status_callback(m_status);
        }
        return UNKNOWN;
    }

    const double duration_s = static_cast<double>(pcm_samples.size()) / WHISPER_SAMPLE_RATE;
    if (logger) logger->info("[WhisperSpeechRecognition] Running transcription ({} thread(s), {} samples / {:.2f} s)...",
                             wparams.n_threads, pcm_samples.size(), duration_s);

    int ret = whisper_full(m_whisperCtx, wparams, pcm_samples.data(), static_cast<int>(pcm_samples.size()));
    if (ret != 0) {
        if (logger) logger->error("[WhisperSpeechRecognition] whisper_full failed with code: {}", ret);
        m_status = speech_recognition_status_e::ERROR;
        if (m_config.status_callback) {
            m_config.status_callback(m_status);
        }
        return UNKNOWN;
    }

    const int n_segments = whisper_full_n_segments(m_whisperCtx);
    if (logger) logger->info("[WhisperSpeechRecognition] Transcription complete — {} segment(s)", n_segments);

    for (int i = 0; i < n_segments; ++i) {
        const int64_t t0 = whisper_full_get_segment_t0(m_whisperCtx, i) * 10;
        const int64_t t1 = whisper_full_get_segment_t1(m_whisperCtx, i) * 10;
        const char* text_ptr = whisper_full_get_segment_text(m_whisperCtx, i);

        TranscriptSegment seg;
        seg.start_time_ms = static_cast<uint64_t>(t0);
        seg.end_time_ms = static_cast<uint64_t>(t1);
        seg.text = text_ptr ? std::string(text_ptr) : "";

        const int n_tokens = whisper_full_n_tokens(m_whisperCtx, i);
        float sum_prob = 0.0f;
        int count = 0;
        for (int j = 0; j < n_tokens; ++j) {
            auto token_data = whisper_full_get_token_data(m_whisperCtx, i, j);
            if (token_data.p > 0.0f) {
                sum_prob += token_data.p;
                count++;
            }
        }
        seg.confidence_score = (count > 0) ? (sum_prob / count) : 1.0f;
        seg.avg_logprob = (count > 0) ? std::log(seg.confidence_score) : 0.0f;

        m_segments.push_back(seg);
    }

    m_status = speech_recognition_status_e::COMPLETED;
    if (m_config.status_callback) {
        m_config.status_callback(m_status);
    }

    return RST_OK;
}

ISpeechRecognition::speech_recognition_status_e WhisperSpeechRecognition::get_status() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_status;
}

rst_code_e WhisperSpeechRecognition::get_result(std::string& text_document_path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_status != speech_recognition_status_e::COMPLETED) {
        return UNKNOWN;
    }

    std::filesystem::path out_path = std::filesystem::temp_directory_path() / "transcript_output.txt";
    std::ofstream ofs(out_path);
    if (!ofs.is_open()) {
        return UNKNOWN;
    }

    for (const auto& seg : m_segments) {
        ofs << seg.text << "\n";
    }
    ofs.close();

    text_document_path = out_path.string();
    m_resultTextPath = text_document_path;
    return RST_OK;
}

rst_code_e WhisperSpeechRecognition::get_segments(std::vector<TranscriptSegment>& out_segments) {
    std::lock_guard<std::mutex> lock(m_mutex);
    out_segments = m_segments;
    return RST_OK;
}
