#ifndef VOICE_QUALITY_ANALYZER_HPP
#define VOICE_QUALITY_ANALYZER_HPP

#include <vector>
#include "speech_recognition/i_speech_recognition.hpp"

struct VoiceQualityMetrics {
    float speech_rate_wpm{0.0f};
    float clarity_score{0.0f};       // 0.0 to 100.0
    float pacing_score{0.0f};        // 0.0 to 100.0
    float overall_quality_score{0.0f}; // 0.0 to 100.0
};

class VoiceQualityAnalyzer {
public:
    /**
     * @brief Computes speech rate in Words Per Minute (WPM) from transcript segments.
     */
    static float calculate_speech_rate(const std::vector<TranscriptSegment>& segments);

    /**
     * @brief Computes clarity score (0-100) based on average token confidence.
     */
    static float calculate_clarity_score(const std::vector<TranscriptSegment>& segments);

    /**
     * @brief Computes pacing score (0-100) based on pause variance between segments.
     */
    static float calculate_pacing_score(const std::vector<TranscriptSegment>& segments);

    /**
     * @brief Performs full voice quality evaluation returning all normalized metrics.
     */
    static VoiceQualityMetrics analyze(const std::vector<TranscriptSegment>& segments);
};

#endif // VOICE_QUALITY_ANALYZER_HPP
