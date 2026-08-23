/**
 * @file diagnostic_radar_evaluator.hpp
 * @brief Multi-Axis Micro-Skill Diagnostic Radar Scoring engine (Pearson Versant / Prova AI architecture).
 */

#ifndef DIAGNOSTIC_RADAR_EVALUATOR_HPP
#define DIAGNOSTIC_RADAR_EVALUATOR_HPP

#include <string>
#include "primitives/definitions.hpp"

/**
 * @struct DiagnosticScorecard
 * @brief 4-Axis Micro-Skill performance scorecard and overall composite evaluation.
 */
struct DiagnosticScorecard {
    float content_recall_score{0.0f};       ///< Content Recall % (weighted key concepts): 0.0 - 100.0%
    float citation_accuracy_score{0.0f};    ///< Citation & Rubric Accuracy % (article/law IDs): 0.0 - 100.0%
    float oral_fluency_score{0.0f};         ///< Oral Fluency Score (Gaussian WPM delivery cadence): 0.0 - 100.0%
    float speech_clarity_score{0.0f};       ///< Speech Clarity Score (acoustic confidence): 0.0 - 100.0%
    float overall_composite_score{0.0f};    ///< Weighted Composite Score across all 4 axes: 0.0 - 100.0%
    float speech_rate_wpm{0.0f};            ///< Raw measured delivery speed in words-per-minute
    std::string assessment_verdict;         ///< Categorical assessment verdict tag
    std::string diagnosis_summary;          ///< Human-readable actionable guidance for the candidate
};

/**
 * @class DiagnosticRadarEvaluator
 * @brief Evaluator that computes the 4-axis multi-skill diagnostic scorecard and assessment verdicts.
 */
class DiagnosticRadarEvaluator {
public:
    /**
     * @brief Computes the oral fluency score (0 - 100%) based on speaking rate in WPM.
     * 
     * @param wpm Measured words per minute.
     * @param optimal_wpm Target ideal speaking rate (default: 135 WPM).
     * @param tolerance Acceptable standard deviation band (default: 25 WPM).
     * @return float Fluency score from 0.0 to 100.0%.
     */
    static float calculate_fluency_score(float wpm, float optimal_wpm = 135.0f, float tolerance = 25.0f);

    /**
     * @brief Evaluates all 4 diagnostic axes and generates the complete diagnostic scorecard.
     * 
     * @param content_recall Weighted content recall score (0 - 100%).
     * @param citation_accuracy Rubric citation accuracy percentage (0 - 100%).
     * @param wpm Measured speech delivery rate in WPM.
     * @param clarity_score Whisper acoustic clarity score (0 - 100%).
     * @param optimal_wpm Optimal speaking cadence in WPM (default: 135 WPM).
     * @return DiagnosticScorecard Complete multi-axis scorecard.
     */
    static DiagnosticScorecard evaluate(
        float content_recall,
        float citation_accuracy,
        float wpm,
        float clarity_score,
        float optimal_wpm = 135.0f
    );
};

#endif // DIAGNOSTIC_RADAR_EVALUATOR_HPP
