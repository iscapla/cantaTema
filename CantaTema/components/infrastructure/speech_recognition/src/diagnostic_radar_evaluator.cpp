/**
 * @file diagnostic_radar_evaluator.cpp
 * @brief Implementation of Multi-Axis Micro-Skill Diagnostic Radar Scoring engine.
 */

#include "speech_recognition/diagnostic_radar_evaluator.hpp"
#include <cmath>
#include <algorithm>

float DiagnosticRadarEvaluator::calculate_fluency_score(float wpm, float optimal_wpm, float tolerance) {
    if (wpm <= 0.0f) {
        return 0.0f;
    }

    // Gaussian bell curve centered at optimal_wpm
    float sigma = (tolerance > 0.0f) ? (tolerance * 1.6f) : 40.0f;
    float delta = wpm - optimal_wpm;
    float exponent = -(delta * delta) / (2.0f * sigma * sigma);
    float score = 100.0f * std::exp(exponent);

    return std::clamp(score, 0.0f, 100.0f);
}

DiagnosticScorecard DiagnosticRadarEvaluator::evaluate(
    float content_recall,
    float citation_accuracy,
    float wpm,
    float clarity_score,
    float optimal_wpm
) {
    DiagnosticScorecard card;
    card.content_recall_score = std::clamp(content_recall, 0.0f, 100.0f);
    card.citation_accuracy_score = std::clamp(citation_accuracy, 0.0f, 100.0f);
    card.speech_rate_wpm = std::max(0.0f, wpm);
    card.speech_clarity_score = std::clamp(clarity_score, 0.0f, 100.0f);
    card.oral_fluency_score = calculate_fluency_score(card.speech_rate_wpm, optimal_wpm);

    // 4-Axis Composite weighting: 35% Recall, 25% Citations, 20% Fluency, 20% Clarity
    card.overall_composite_score = (0.35f * card.content_recall_score) +
                                   (0.25f * card.citation_accuracy_score) +
                                   (0.20f * card.oral_fluency_score) +
                                   (0.20f * card.speech_clarity_score);
    card.overall_composite_score = std::clamp(card.overall_composite_score, 0.0f, 100.0f);

    // Determine verdict & actionable diagnostic summary
    if (card.overall_composite_score >= 85.0f) {
        card.assessment_verdict = "EXCELLENT_READINESS";
        card.diagnosis_summary = "Outstanding oral delivery: high concept recall, precise domain citations, and optimal pacing.";
    } else if (card.overall_composite_score >= 70.0f) {
        if (card.citation_accuracy_score < 70.0f) {
            card.assessment_verdict = "REINFORCE_CITATIONS";
            card.diagnosis_summary = "Good conceptual coverage, but critical article numbers and statutory citations were omitted.";
        } else if (card.oral_fluency_score < 70.0f) {
            card.assessment_verdict = "ADJUST_PACING";
            card.diagnosis_summary = "Strong content retention, but delivery pace diverged from optimal speaking cadence.";
        } else {
            card.assessment_verdict = "SOLID_PROGRESS";
            card.diagnosis_summary = "Solid performance across all diagnostic dimensions with minor room for refinement.";
        }
    } else if (card.overall_composite_score >= 50.0f) {
        card.assessment_verdict = "NEEDS_PRACTICE";
        card.diagnosis_summary = "Moderate topic coverage with notable omissions and pacing variance. Additional oral practice advised.";
    } else {
        card.assessment_verdict = "CRITICAL_REVISION_REQUIRED";
        card.diagnosis_summary = "Substantial topic omissions and low citation accuracy. Comprehensive reference restudy required.";
    }

    return card;
}
