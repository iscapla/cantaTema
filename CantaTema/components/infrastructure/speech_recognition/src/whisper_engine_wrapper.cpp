#include "speech_recognition/whisper_engine_wrapper.hpp"

#include <cmath>
#include <algorithm>
#include "whisper.h"
#include "primitives/utils_logger.hpp"

whisper_context* WhisperEngineWrapper::init_from_file_with_params(const std::string& model_path, bool use_gpu) {
    struct whisper_context_params cparams = whisper_context_default_params();
    cparams.use_gpu = use_gpu;
    cparams.flash_attn = false;

    return whisper_init_from_file_with_params(model_path.c_str(), cparams);
}

void WhisperEngineWrapper::free_context(whisper_context* ctx) {
    if (ctx) {
        whisper_free(ctx);
    }
}

int WhisperEngineWrapper::run_full(whisper_context* ctx, const std::string& language, const std::vector<float>& pcm_samples) {
    return run_full(ctx, language, pcm_samples, nullptr);
}

int WhisperEngineWrapper::run_full(whisper_context* ctx, const std::string& language, const std::vector<float>& pcm_samples, std::function<void(int)> progress_cb) {
    if (!ctx) return -1;

    struct whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    std::string lang = language.empty() ? "es" : language;
    wparams.language        = lang.c_str();
    wparams.n_threads       = 8;
    wparams.duration_ms     = 0;
    wparams.print_progress  = false;
    wparams.print_special   = false;
    wparams.print_realtime  = false;
    wparams.print_timestamps = false;
    wparams.no_timestamps   = false;
    wparams.single_segment  = false;
    wparams.translate       = false;
    wparams.token_timestamps = true;
    wparams.temperature_inc = 0.0f;
    wparams.greedy.best_of  = 1;
    wparams.no_context      = true;

    struct ProgressData {
        std::function<void(int)> cb;
    };
    ProgressData cb_data{progress_cb};

    wparams.progress_callback_user_data = &cb_data;
    wparams.progress_callback = [](struct whisper_context*, struct whisper_state*, int progress, void* user_data) {
        static int last_reported_progress = -1;
        if (progress != last_reported_progress) {
            if (user_data) {
                auto* data = static_cast<ProgressData*>(user_data);
                if (data->cb) {
                    data->cb(progress);
                }
            }
            last_reported_progress = progress;
        }
    };

    return whisper_full(ctx, wparams, pcm_samples.data(), static_cast<int>(pcm_samples.size()));
}

std::vector<WhisperSegmentData> WhisperEngineWrapper::extract_segments(whisper_context* ctx) {
    std::vector<WhisperSegmentData> segments;
    if (!ctx) return segments;

    const int n_segments = whisper_full_n_segments(ctx);
    segments.reserve(n_segments);

    for (int i = 0; i < n_segments; ++i) {
        const int64_t t0 = whisper_full_get_segment_t0(ctx, i) * 10;
        const int64_t t1 = whisper_full_get_segment_t1(ctx, i) * 10;
        const char* text_ptr = whisper_full_get_segment_text(ctx, i);

        WhisperSegmentData seg;
        seg.t0 = t0;
        seg.t1 = t1;
        seg.text = text_ptr ? std::string(text_ptr) : "";

        const int n_tokens = whisper_full_n_tokens(ctx, i);
        float sum_prob = 0.0f;
        int count = 0;
        for (int j = 0; j < n_tokens; ++j) {
            auto token_data = whisper_full_get_token_data(ctx, i, j);
            if (token_data.p > 0.0f) {
                sum_prob += token_data.p;
                count++;
            }
        }
        seg.avg_confidence = (count > 0) ? (sum_prob / count) : 1.0f;
        seg.avg_logprob = (count > 0) ? std::log(seg.avg_confidence) : 0.0f;

        segments.push_back(seg);
    }

    return segments;
}
