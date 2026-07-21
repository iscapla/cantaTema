#include <gtest/gtest.h>
#include "speech_recognition/voice_quality_analyzer.hpp"

TEST(VoiceQualityAnalyzerTest, EmptySegments) {
    std::vector<TranscriptSegment> segments;
    auto metrics = VoiceQualityAnalyzer::analyze(segments);

    EXPECT_FLOAT_EQ(metrics.speech_rate_wpm, 0.0f);
    EXPECT_FLOAT_EQ(metrics.clarity_score, 0.0f);
    EXPECT_FLOAT_EQ(metrics.pacing_score, 100.0f);
    EXPECT_GE(metrics.overall_quality_score, 0.0f);
    EXPECT_LE(metrics.overall_quality_score, 100.0f);
}

TEST(VoiceQualityAnalyzerTest, SingleSegment) {
    std::vector<TranscriptSegment> segments = {
        {0, 5000, "Hola a todos, esta es una prueba de voz.", 0.0f, 0.95f}
    };

    auto metrics = VoiceQualityAnalyzer::analyze(segments);
    EXPECT_GT(metrics.speech_rate_wpm, 0.0f);
    EXPECT_FLOAT_EQ(metrics.clarity_score, 95.0f);
    EXPECT_FLOAT_EQ(metrics.pacing_score, 100.0f);
    EXPECT_GT(metrics.overall_quality_score, 0.0f);
}

TEST(VoiceQualityAnalyzerTest, MultipleSegmentsPacingAndClarity) {
    std::vector<TranscriptSegment> segments = {
        {0, 2000, "El primer segmento de la exposicion.", 0.0f, 0.90f},
        {2500, 4500, "El segundo segmento continua de forma clara.", 0.0f, 0.92f},
        {5000, 7000, "Y conclusion final de la respuesta.", 0.0f, 0.88f}
    };

    float speech_rate = VoiceQualityAnalyzer::calculate_speech_rate(segments);
    float clarity = VoiceQualityAnalyzer::calculate_clarity_score(segments);
    float pacing = VoiceQualityAnalyzer::calculate_pacing_score(segments);
    auto metrics = VoiceQualityAnalyzer::analyze(segments);

    EXPECT_GT(speech_rate, 0.0f);
    EXPECT_NEAR(clarity, 90.0f, 2.0f);
    EXPECT_GT(pacing, 50.0f);
    EXPECT_GT(metrics.overall_quality_score, 50.0f);
}

TEST(VoiceQualityAnalyzerTest, IrregularPacingScoresLower) {
    std::vector<TranscriptSegment> regular_segments = {
        {0, 2000, "Fragmento uno", 0.0f, 0.9f},
        {2500, 4500, "Fragmento dos", 0.0f, 0.9f},
        {5000, 7000, "Fragmento tres", 0.0f, 0.9f}
    };

    std::vector<TranscriptSegment> irregular_segments = {
        {0, 2000, "Fragmento uno", 0.0f, 0.9f},
        {2100, 4100, "Fragmento dos", 0.0f, 0.9f},
        {10000, 12000, "Fragmento tres", 0.0f, 0.9f}
    };

    float regular_pacing = VoiceQualityAnalyzer::calculate_pacing_score(regular_segments);
    float irregular_pacing = VoiceQualityAnalyzer::calculate_pacing_score(irregular_segments);

    EXPECT_GT(regular_pacing, irregular_pacing);
}
