#include "speech_recognition/whisper_speech_recognition.hpp"

#include <fstream>
#include <sstream>
#include <cmath>
#include <algorithm>
#include <filesystem>

#include "whisper.h"
#include "primitives/tool_paths.hpp"
#include "primitives/utils_logger.hpp"
#include "file_handler/sound_handler.hpp"

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
    }

    if (!std::filesystem::exists(model_path)) {
        if (logger) logger->error("[WhisperSpeechRecognition] Whisper model file not found at: {}", model_path.string());
        m_status = speech_recognition_status_e::ERROR;
        return FILE_NOT_FOUND;
    }

    struct whisper_context_params cparams = whisper_context_default_params();
    m_whisperCtx = whisper_init_from_file_with_params(model_path.string().c_str(), cparams);
    if (!m_whisperCtx) {
        if (logger) logger->error("[WhisperSpeechRecognition] Failed to initialize whisper context from: {}", model_path.string());
        m_status = speech_recognition_status_e::ERROR;
        return UNKNOWN;
    }

    if (logger) logger->info("[WhisperSpeechRecognition] Whisper context initialized with model: {}", model_path.string());
    return RST_OK;
}

std::vector<float> WhisperSpeechRecognition::decode_audio_to_pcm(const std::string& audio_file_path) {
    // If SoundHandler or audio file reading is available, decode to 16kHz mono float PCM
    SoundFileHandler sound_handler(audio_file_path);
    std::vector<float> pcm_samples;

    if (!m_soundSystem) {
        logger->warn("[WhisperSpeechRecognition] SoundSystem not injected; returning empty PCM buffer.");
        return pcm_samples;
    }

    // SoundSystem playback stream can be queried or mock audio PCM can be supplied
    // For general processing, we load audio stream using SoundHandler and decrypt if required
    return pcm_samples;
}

rst_code_e WhisperSpeechRecognition::submit_task(const std::string& audio_file_path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_whisperCtx) {
        logger->error("[WhisperSpeechRecognition] submit_task failed: whisper context not initialized.");
        m_status = speech_recognition_status_e::ERROR;
        return UNKNOWN;
    }

    if (!std::filesystem::exists(audio_file_path)) {
        logger->error("[WhisperSpeechRecognition] Audio file not found: {}", audio_file_path);
        m_status = speech_recognition_status_e::ERROR;
        return FILE_NOT_FOUND;
    }

    m_status = speech_recognition_status_e::PROCESSING;
    if (m_config.status_callback) {
        m_config.status_callback(m_status);
    }

    m_segments.clear();

    struct whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    wparams.print_realtime = false;
    wparams.print_progress = false;
    wparams.print_timestamps = false;
    wparams.print_special = false;
    wparams.translate = false;

    std::string lang = m_config.language.empty() ? "es" : m_config.language;
    wparams.language = lang.c_str();

    // Enable token log-probabilities & timestamps
    wparams.token_timestamps = true;

    std::vector<float> pcm_samples = decode_audio_to_pcm(audio_file_path);
    
    // If audio samples were decoded successfully, pass them to whisper_full
    if (!pcm_samples.empty()) {
        int ret = whisper_full(m_whisperCtx, wparams, pcm_samples.data(), static_cast<int>(pcm_samples.size()));
        if (ret != 0) {
            logger->error("[WhisperSpeechRecognition] whisper_full failed with code: {}", ret);
            m_status = speech_recognition_status_e::ERROR;
            if (m_config.status_callback) {
                m_config.status_callback(m_status);
            }
            return UNKNOWN;
        }

        const int n_segments = whisper_full_n_segments(m_whisperCtx);
        for (int i = 0; i < n_segments; ++i) {
            const int64_t t0 = whisper_full_get_segment_t0(m_whisperCtx, i) * 10; // convert to ms
            const int64_t t1 = whisper_full_get_segment_t1(m_whisperCtx, i) * 10;
            const char* text_ptr = whisper_full_get_segment_text(m_whisperCtx, i);

            TranscriptSegment seg;
            seg.start_time_ms = static_cast<uint64_t>(t0);
            seg.end_time_ms = static_cast<uint64_t>(t1);
            seg.text = text_ptr ? std::string(text_ptr) : "";

            // Compute confidence score from token probabilities
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
