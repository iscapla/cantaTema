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
    EXPECT_NE(html.find("clearSelection"), std::string::npos);
    EXPECT_NE(html.find("active-highlight"), std::string::npos);
}

TEST(TextComparisonVisualizerTest, GenerateHtmlWithRadarAndRubricChecklist) {
    TextComparisonVisualizer visualizer;
    TextComparisonInput input;
    input.document_title = "Derecho Constitucional";
    input.reference_filepath = "tema1.pdf";
    input.audio_filepath = "tema1_practice.opus";
    input.overall_coverage_pct = 85.0;
    input.total_ref_chunks = 2;
    input.mentioned_chunks = 2;
    input.active_domain_badge = "⚠️ MISSING LEGAL CITATION";

    // Set 4-axis diagnostic scorecard
    input.diagnostic_scores.content_recall_score = 85.0f;
    input.diagnostic_scores.citation_accuracy_score = 100.0f;
    input.diagnostic_scores.oral_fluency_score = 92.0f;
    input.diagnostic_scores.speech_clarity_score = 88.0f;
    input.diagnostic_scores.overall_composite_score = 89.5f;

    // Set Rubric Scorecard
    RubricItem rub1;
    rub1.item_id = 0;
    rub1.entity_type = RubricEntityType::LEGAL_ARTICLE;
    rub1.raw_text = "Artículo 14";
    rub1.normalized_text = "articulo 14";
    rub1.entity_label = "Article 14";
    rub1.is_satisfied = true;
    rub1.ref_chunk_index = 0;

    RubricItem rub2;
    rub2.item_id = 1;
    rub2.entity_type = RubricEntityType::LAW_ID;
    rub2.raw_text = "Ley 39/2015";
    rub2.normalized_text = "ley 39/2015";
    rub2.entity_label = "Ley 39/2015";
    rub2.is_satisfied = false;
    rub2.ref_chunk_index = 1;

    input.rubric_scorecard.total_items = 2;
    input.rubric_scorecard.satisfied_items = 1;
    input.rubric_scorecard.omitted_items = 1;
    input.rubric_scorecard.citation_accuracy_pct = 50.0f;
    input.rubric_scorecard.items = {rub1, rub2};

    // Reference Items with phonetic token and missing citation
    TextComparisonInput::ReferenceItem r1;
    r1.id = 0;
    r1.text = "El Artículo 14 reconoce la igualdad.";
    r1.coverage_status = coverage_level_e::MENTIONED;
    r1.similarity_score = 0.95f;
    r1.matched_transcript_index = 0;

    WordDiffToken t_match;
    t_match.original_word = "El";
    t_match.status = WordDiffStatus::MATCHED;

    WordDiffToken t_phonetic;
    t_phonetic.original_word = "igualdad";
    t_phonetic.status = WordDiffStatus::PHONETIC_MISPRONUNCIATION;

    r1.word_alignment.reference_words = {t_match, t_phonetic};
    r1.word_alignment.total_reference_weight = 2.0f;
    r1.word_alignment.weighted_recall_score = 0.90f;

    TextComparisonInput::ReferenceItem r2;
    r2.id = 1;
    r2.text = "Conforme a la Ley 39/2015 se tramita.";
    r2.coverage_status = coverage_level_e::NOT_MENTIONED;
    r2.similarity_score = 0.20f;
    r2.matched_transcript_index = -1;

    WordDiffToken t_omitted_law;
    t_omitted_law.original_word = "Ley";
    t_omitted_law.is_legal_citation = true;
    t_omitted_law.status = WordDiffStatus::OMITTED;

    r2.word_alignment.reference_words = {t_omitted_law};
    r2.word_alignment.has_missing_legal_citation = true;

    input.reference_items = {r1, r2};

    std::string html;
    rst_code_e res = visualizer.generate_html(input, html);

    EXPECT_EQ(res, RST_OK);
    EXPECT_FALSE(html.empty());

    // Verify SVG radar chart presence
    EXPECT_NE(html.find("<polygon points="), std::string::npos);
    EXPECT_NE(html.find("Recall"), std::string::npos);
    EXPECT_NE(html.find("Cit"), std::string::npos);
    EXPECT_NE(html.find("Fluency"), std::string::npos);
    EXPECT_NE(html.find("Clarity"), std::string::npos);

    // Verify Rubric Checklist Drawer
    EXPECT_NE(html.find("Exam Rubric Checklist"), std::string::npos);
    EXPECT_NE(html.find("Article 14"), std::string::npos);
    EXPECT_NE(html.find("Ley 39/2015"), std::string::npos);
    EXPECT_NE(html.find("data-filter=\"satisfied\""), std::string::npos);

    // Verify Phonetic Mispronunciation token rendering
    EXPECT_NE(html.find("underline wavy"), std::string::npos);

    // Verify Custom Domain Badge
    EXPECT_NE(html.find("⚠️ MISSING LEGAL CITATION"), std::string::npos);
}

