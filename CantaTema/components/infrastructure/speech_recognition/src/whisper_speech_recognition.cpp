#include "speech_recognition/whisper_speech_recognition.hpp"
#include "speech_recognition/i_whisper_engine_wrapper.hpp"
#include "speech_recognition/whisper_engine_wrapper.hpp"

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
#include "sound_system/sound_converter.hpp"

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



} // anonymous namespace

WhisperSpeechRecognition::WhisperSpeechRecognition(
    std::shared_ptr<ISoundSystem> sound_system,
    std::shared_ptr<IWhisperEngineWrapper> engine_wrapper,
    std::shared_ptr<cantatema::infra::IGpuDetector> gpu_detector
) : m_soundSystem(sound_system), m_engineWrapper(engine_wrapper), m_gpuDetector(gpu_detector) {
    if (!m_engineWrapper) {
        m_engineWrapper = std::make_shared<WhisperEngineWrapper>();
    }
    if (!m_gpuDetector) {
        m_gpuDetector = std::make_shared<cantatema::infra::GpuDetector>();
    }
}

WhisperSpeechRecognition::~WhisperSpeechRecognition() {
    if (m_whisperCtx && m_engineWrapper) {
        m_engineWrapper->free_context(m_whisperCtx);
        m_whisperCtx = nullptr;
    }
}

rst_code_e WhisperSpeechRecognition::initialize(const speech_recognition_config_t& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config = config;
    m_status = speech_recognition_status_e::IDLE;

    if (m_whisperCtx && m_engineWrapper) {
        m_engineWrapper->free_context(m_whisperCtx);
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

    cantatema::infra::AccelerationReport accel_report = m_gpuDetector ? m_gpuDetector->detect_accelerators() : cantatema::infra::detect_accelerators();

    bool request_gpu = config.use_gpu;

    if (logger) {
        if (request_gpu && !accel_report.use_gpu) {
            logger->warn("[WhisperSpeechRecognition] GPU requested in config, but no registered GGML GPU backend (CUDA/Vulkan) was found. Attempting GPU context initialization...");
        } else {
            logger->info("[WhisperSpeechRecognition] Initializing whisper context (Strategy: {}, use_gpu: {})",
                         accel_report.selected_device_name, request_gpu ? "ON" : "OFF");
        }
    }

    m_whisperCtx = m_engineWrapper->init_from_file_with_params(model_path.string(), request_gpu);
    if (!m_whisperCtx && request_gpu) {
        if (logger) logger->warn("[WhisperSpeechRecognition] GPU context initialization failed. Retrying with CPU mode...");
        m_whisperCtx = m_engineWrapper->init_from_file_with_params(model_path.string(), false);
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

        if (!SoundConverter::convert_opus_to_wav(audio_file_path, temp_wav.string())) {
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
    if (logger) logger->info("[WhisperSpeechRecognition] Running transcription ({} samples / {:.2f} s)...",
                             pcm_samples.size(), duration_s);

    std::string lang = m_config.language.empty() ? "es" : m_config.language;
    int ret = m_config.progress_callback ?
        m_engineWrapper->run_full(m_whisperCtx, lang, pcm_samples, m_config.progress_callback) :
        m_engineWrapper->run_full(m_whisperCtx, lang, pcm_samples);
    if (ret != 0) {
        if (logger) logger->error("[WhisperSpeechRecognition] whisper_full failed with code: {}", ret);
        m_status = speech_recognition_status_e::ERROR;
        if (m_config.status_callback) {
            m_config.status_callback(m_status);
        }
        return UNKNOWN;
    }

    auto raw_segments = m_engineWrapper->extract_segments(m_whisperCtx);
    if (logger) logger->info("[WhisperSpeechRecognition] Transcription complete — {} segment(s)", raw_segments.size());

    for (const auto& raw_seg : raw_segments) {
        TranscriptSegment seg;
        seg.start_time_ms = static_cast<uint64_t>(raw_seg.t0);
        seg.end_time_ms = static_cast<uint64_t>(raw_seg.t1);
        seg.text = raw_seg.text;
        seg.confidence_score = raw_seg.avg_confidence;
        seg.avg_logprob = raw_seg.avg_logprob;

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
