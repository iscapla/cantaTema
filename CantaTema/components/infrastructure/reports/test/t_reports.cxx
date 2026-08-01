#include <gtest/gtest.h>
#include "reports/whisper_accuracy_visualizer.hpp"
#include "reports/text_comparison_visualizer.hpp"

TEST(WhisperAccuracyVisualizerTest, GenerateHtmlBasic) {
    WhisperAccuracyVisualizer visualizer;
    WhisperAccuracyInput input;
    input.audio_filepath = "test_audio.opus";
    input.model_name = "ggml-medium.bin";
    input.language = "es";
    input.total_duration_ms = 12500;
    input.processing_time_ms = 450;
    input.speech_rate_wpm = 135.5f;
    input.clarity_score = 92.0f;
    input.overall_confidence = 0.88f;

    TranscriptSegment s1;
    s1.start_time_ms = 0;
    s1.end_time_ms = 4500;
    s1.text = "Hola buenos dias estudiantes";
    s1.confidence_score = 0.95f;
    s1.avg_logprob = -0.15f;

    TranscriptSegment s2;
    s2.start_time_ms = 4600;
    s2.end_time_ms = 8500;
    s2.text = "Hoy repasaremos la leccion de historia";
    s2.confidence_score = 0.72f;
    s2.avg_logprob = -0.45f;

    TranscriptSegment s3;
    s3.start_time_ms = 8600;
    s3.end_time_ms = 12500;
    s3.text = "Algunos detalles no estaban claros";
    s3.confidence_score = 0.45f;
    s3.avg_logprob = -0.85f;

    input.segments = {s1, s2, s3};

    std::string html;
    rst_code_e res = visualizer.generate_html(input, html);

    EXPECT_EQ(res, RST_OK);
    EXPECT_FALSE(html.empty());

    // Check top comment metadata
    EXPECT_NE(html.find("<!--"), std::string::npos);
    EXPECT_NE(html.find("CANTA TEMA - WHISPER TRANSCRIPTION ACCURACY REPORT"), std::string::npos);
    EXPECT_NE(html.find("test_audio.opus"), std::string::npos);
    EXPECT_NE(html.find("ggml-medium.bin"), std::string::npos);

    // Check 3 color level CSS classes
    EXPECT_NE(html.find("lvl-high"), std::string::npos);
    EXPECT_NE(html.find("lvl-medium"), std::string::npos);
    EXPECT_NE(html.find("lvl-low"), std::string::npos);

    // Check segment texts
    EXPECT_NE(html.find("Hola buenos dias estudiantes"), std::string::npos);
    EXPECT_NE(html.find("Hoy repasaremos la leccion de historia"), std::string::npos);
    EXPECT_NE(html.find("Algunos detalles no estaban claros"), std::string::npos);
}

TEST(TextComparisonVisualizerTest, GenerateHtmlDualColumn) {
    TextComparisonVisualizer visualizer;
    TextComparisonInput input;
    input.document_title = "Don Quijote Capítulo 1";
    input.reference_filepath = "quijote.pdf";
    input.audio_filepath = "quijote_practice.opus";
    input.whisper_model = "ggml-medium.bin";
    input.llama_model = "multilingual-e5-large";
    input.overall_coverage_pct = 66.7;
    input.total_ref_chunks = 3;
    input.mentioned_chunks = 1;
    input.not_clear_chunks = 1;
    input.not_mentioned_chunks = 1;

    TextComparisonInput::ReferenceItem r1;
    r1.id = 0;
    r1.text = "En un lugar de la Mancha, de cuyo nombre no quiero acordarme";
    r1.importance_weight = 1.2f;
    r1.coverage_status = coverage_level_e::MENTIONED;
    r1.similarity_score = 0.89f;
    r1.matched_transcript_index = 0;

    TextComparisonInput::ReferenceItem r2;
    r2.id = 1;
    r2.text = "no ha mucho tiempo que vivía un hidalgo de los de lanza en astillero";
    r2.importance_weight = 1.0f;
    r2.coverage_status = coverage_level_e::NOT_CLEAR;
    r2.similarity_score = 0.62f;
    r2.matched_transcript_index = 1;

    TextComparisonInput::ReferenceItem r3;
    r3.id = 2;
    r3.text = "Una olla de algo más vaca que carnero, salpicón las más noches";
    r3.importance_weight = 1.0f;
    r3.coverage_status = coverage_level_e::NOT_MENTIONED;
    r3.similarity_score = 0.25f;
    r3.matched_transcript_index = -1;

    input.reference_items = {r1, r2, r3};

    TextComparisonInput::TranscriptItem t1;
    t1.id = 0;
    t1.start_time_ms = 0;
    t1.end_time_ms = 4000;
    t1.text = "en un lugar de la mancha cuyo nombre no me acuerdo";
    t1.confidence_score = 0.92f;
    t1.primary_matched_ref_index = 0;

    TextComparisonInput::TranscriptItem t2;
    t2.id = 1;
    t2.start_time_ms = 4100;
    t2.end_time_ms = 8000;
    t2.text = "un hidalgo con lanza y escudo";
    t2.confidence_score = 0.85f;
    t2.primary_matched_ref_index = 1;

    input.transcript_items = {t1, t2};

    std::string html;
    rst_code_e res = visualizer.generate_html(input, html);

    EXPECT_EQ(res, RST_OK);
    EXPECT_FALSE(html.empty());

    // Check header comment and title
    EXPECT_NE(html.find("CANTA TEMA - DUAL-COLUMN TEXT COMPARISON REPORT"), std::string::npos);
    EXPECT_NE(html.find("Don Quijote Capítulo 1"), std::string::npos);

    // Check stats metrics
    EXPECT_NE(html.find("66.7%"), std::string::npos);

    // Check coverage status badges
    EXPECT_NE(html.find("MENTIONED"), std::string::npos);
    EXPECT_NE(html.find("NOT CLEAR"), std::string::npos);
    EXPECT_NE(html.find("NOT MENTIONED"), std::string::npos);

    // Check elements and data attributes
    EXPECT_NE(html.find("data-ref-id=\"0\""), std::string::npos);
    EXPECT_NE(html.find("data-match-ts-idx=\"0\""), std::string::npos);
    EXPECT_NE(html.find("data-ts-id=\"0\""), std::string::npos);

    // Check JS script inclusion
    EXPECT_NE(html.find("isSyncingScroll"), std::string::npos);
    EXPECT_NE(html.find("active-highlight"), std::string::npos);
}
