#include "speech_recognition/voice_quality_analyzer.hpp"

#include <sstream>
#include <numeric>
#include <cmath>
#include <algorithm>

namespace {
    size_t count_words(const std::string& text) {
        std::istringstream iss(text);
        std::string word;
        size_t count = 0;
        while (iss >> word) {
            count++;
        }
        return count;
    }
}

float VoiceQualityAnalyzer::calculate_speech_rate(const std::vector<TranscriptSegment>& segments) {
    if (segments.empty()) return 0.0f;

    size_t total_words = 0;
    for (const auto& seg : segments) {
        total_words += count_words(seg.text);
    }

    uint64_t start_ms = segments.front().start_time_ms;
    uint64_t end_ms = segments.back().end_time_ms;
    if (end_ms <= start_ms) return 0.0f;

    double duration_minutes = static_cast<double>(end_ms - start_ms) / 60000.0;
    if (duration_minutes <= 0.0) return 0.0f;

    return static_cast<float>(total_words / duration_minutes);
}

float VoiceQualityAnalyzer::calculate_clarity_score(const std::vector<TranscriptSegment>& segments) {
    if (segments.empty()) return 0.0f;

    float sum_confidence = 0.0f;
    for (const auto& seg : segments) {
        sum_confidence += seg.confidence_score;
    }

    float avg_confidence = sum_confidence / static_cast<float>(segments.size());
    return std::clamp(avg_confidence * 100.0f, 0.0f, 100.0f);
}

float VoiceQualityAnalyzer::calculate_pacing_score(const std::vector<TranscriptSegment>& segments) {
    if (segments.size() <= 1) return 100.0f;

    std::vector<float> pause_durations_sec;
    for (size_t i = 1; i < segments.size(); ++i) {
        if (segments[i].start_time_ms >= segments[i - 1].end_time_ms) {
            float pause = static_cast<float>(segments[i].start_time_ms - segments[i - 1].end_time_ms) / 1000.0f;
            pause_durations_sec.push_back(pause);
        }
    }

    if (pause_durations_sec.empty()) return 100.0f;

    float sum = std::accumulate(pause_durations_sec.begin(), pause_durations_sec.end(), 0.0f);
    float mean = sum / pause_durations_sec.size();

    float sq_sum = 0.0f;
    for (float p : pause_durations_sec) {
        sq_sum += (p - mean) * (p - mean);
    }
    float variance = sq_sum / pause_durations_sec.size();
    float stddev = std::sqrt(variance);

    float pacing_score = 100.0f / (1.0f + stddev);
    return std::clamp(pacing_score, 0.0f, 100.0f);
}

VoiceQualityMetrics VoiceQualityAnalyzer::analyze(const std::vector<TranscriptSegment>& segments) {
    VoiceQualityMetrics metrics;
    metrics.speech_rate_wpm = calculate_speech_rate(segments);
    metrics.clarity_score = calculate_clarity_score(segments);
    metrics.pacing_score = calculate_pacing_score(segments);

    // Normalize speech rate (optimal range ~ 120-160 WPM for speaking/presentation)
    float wpm_score = 100.0f;
    if (metrics.speech_rate_wpm < 80.0f) {
        wpm_score = std::max(0.0f, (metrics.speech_rate_wpm / 80.0f) * 100.0f);
    } else if (metrics.speech_rate_wpm > 200.0f) {
        wpm_score = std::max(0.0f, 100.0f - ((metrics.speech_rate_wpm - 200.0f) / 2.0f));
    }

    metrics.overall_quality_score = (metrics.clarity_score * 0.5f) + 
                                    (metrics.pacing_score * 0.3f) + 
                                    (wpm_score * 0.2f);
    metrics.overall_quality_score = std::clamp(metrics.overall_quality_score, 0.0f, 100.0f);

    return metrics;
}
