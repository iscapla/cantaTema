/**
 * @file t_diagnostic_radar_evaluator.cxx
 * @brief Unit tests for DiagnosticRadarEvaluator 4-axis multi-skill scoring engine.
 */

#include <gtest/gtest.h>
#include "speech_recognition/diagnostic_radar_evaluator.hpp"

TEST(DiagnosticRadarEvaluatorTest, FluencyCalculation) {
    // Optimal WPM gives 100%
    float score_opt = DiagnosticRadarEvaluator::calculate_fluency_score(135.0f, 135.0f);
    EXPECT_NEAR(score_opt, 100.0f, 0.1f);

    // Acceptable pace gives >= 80%
    float score_120 = DiagnosticRadarEvaluator::calculate_fluency_score(120.0f, 135.0f);
    float score_150 = DiagnosticRadarEvaluator::calculate_fluency_score(150.0f, 135.0f);
    EXPECT_GT(score_120, 80.0f);
    EXPECT_GT(score_150, 80.0f);

    // Too slow or too fast gives lower score
    float score_slow = DiagnosticRadarEvaluator::calculate_fluency_score(50.0f, 135.0f);
    float score_fast = DiagnosticRadarEvaluator::calculate_fluency_score(240.0f, 135.0f);
    EXPECT_LT(score_slow, 40.0f);
    EXPECT_LT(score_fast, 40.0f);

    // Edge cases: zero or negative
    EXPECT_FLOAT_EQ(DiagnosticRadarEvaluator::calculate_fluency_score(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(DiagnosticRadarEvaluator::calculate_fluency_score(-10.0f), 0.0f);
}

TEST(DiagnosticRadarEvaluatorTest, EvaluateExcellentReadiness) {
    auto card = DiagnosticRadarEvaluator::evaluate(92.0f, 95.0f, 135.0f, 90.0f);

    EXPECT_GE(card.overall_composite_score, 85.0f);
    EXPECT_EQ(card.assessment_verdict, "EXCELLENT_READINESS");
    EXPECT_FALSE(card.diagnosis_summary.empty());
    EXPECT_FLOAT_EQ(card.content_recall_score, 92.0f);
    EXPECT_FLOAT_EQ(card.citation_accuracy_score, 95.0f);
    EXPECT_NEAR(card.oral_fluency_score, 100.0f, 0.1f);
    EXPECT_FLOAT_EQ(card.speech_clarity_score, 90.0f);
}

TEST(DiagnosticRadarEvaluatorTest, EvaluateReinforceCitations) {
    // Good recall, fluency, and clarity, but low citation score
    auto card = DiagnosticRadarEvaluator::evaluate(85.0f, 40.0f, 135.0f, 88.0f);

    EXPECT_GE(card.overall_composite_score, 70.0f);
    EXPECT_EQ(card.assessment_verdict, "REINFORCE_CITATIONS");
}

TEST(DiagnosticRadarEvaluatorTest, EvaluateAdjustPacing) {
    // Good recall, citations, and clarity, but speech is rushed (230 WPM)
    auto card = DiagnosticRadarEvaluator::evaluate(90.0f, 90.0f, 230.0f, 90.0f);

    EXPECT_GE(card.overall_composite_score, 70.0f);
    EXPECT_EQ(card.assessment_verdict, "ADJUST_PACING");
}

TEST(DiagnosticRadarEvaluatorTest, EvaluateNeedsPracticeAndCriticalRevision) {
    // Moderate score
    auto card_mid = DiagnosticRadarEvaluator::evaluate(55.0f, 50.0f, 100.0f, 60.0f);
    EXPECT_GE(card_mid.overall_composite_score, 50.0f);
    EXPECT_LT(card_mid.overall_composite_score, 70.0f);
    EXPECT_EQ(card_mid.assessment_verdict, "NEEDS_PRACTICE");

    // Critical score
    auto card_low = DiagnosticRadarEvaluator::evaluate(20.0f, 10.0f, 50.0f, 30.0f);
    EXPECT_LT(card_low.overall_composite_score, 50.0f);
    EXPECT_EQ(card_low.assessment_verdict, "CRITICAL_REVISION_REQUIRED");
}

TEST(DiagnosticRadarEvaluatorTest, ClampingAndBoundaries) {
    // Over-100 values should be clamped
    auto card = DiagnosticRadarEvaluator::evaluate(150.0f, 120.0f, 135.0f, 110.0f);
    EXPECT_FLOAT_EQ(card.content_recall_score, 100.0f);
    EXPECT_FLOAT_EQ(card.citation_accuracy_score, 100.0f);
    EXPECT_FLOAT_EQ(card.speech_clarity_score, 100.0f);
    EXPECT_FLOAT_EQ(card.overall_composite_score, 100.0f);
}
