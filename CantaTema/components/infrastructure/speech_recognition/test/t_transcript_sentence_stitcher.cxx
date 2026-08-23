/**
 * @file t_transcript_sentence_stitcher.cxx
 * @brief Unit tests for TranscriptSentenceStitcher heuristic sentence reconstruction.
 */

#include <gtest/gtest.h>
#include "speech_recognition/transcript_sentence_stitcher.hpp"

TEST(TranscriptSentenceStitcherTest, CountWords) {
    EXPECT_EQ(TranscriptSentenceStitcher::count_words(""), 0);
    EXPECT_EQ(TranscriptSentenceStitcher::count_words("   "), 0);
    EXPECT_EQ(TranscriptSentenceStitcher::count_words("Hola mundo"), 2);
    EXPECT_EQ(TranscriptSentenceStitcher::count_words("  Uno   dos  tres cuatro  "), 4);
}

TEST(TranscriptSentenceStitcherTest, ContinuationDetection) {
    TranscriptSegment seg_comma;
    seg_comma.start_time_ms = 0;
    seg_comma.end_time_ms = 3000;
    seg_comma.text = "En primer lugar,";

    TranscriptSegment seg_cont;
    seg_cont.start_time_ms = 3400; // 400ms pause
    seg_cont.end_time_ms = 6000;
    seg_cont.text = "debemos analizar la situación actual.";

    // Comma + short gap -> true
    EXPECT_TRUE(TranscriptSentenceStitcher::is_continuation(seg_comma, seg_cont, 3));

    // Long pause (> 1500ms) -> false
    TranscriptSegment seg_late = seg_cont;
    seg_late.start_time_ms = 5000; // 2000ms pause
    EXPECT_FALSE(TranscriptSentenceStitcher::is_continuation(seg_comma, seg_late, 3));

    // Period with uppercase continuation -> false
    TranscriptSegment seg_period;
    seg_period.start_time_ms = 0;
    seg_period.end_time_ms = 3000;
    seg_period.text = "Este es el primer punto.";

    TranscriptSegment seg_new_sent;
    seg_new_sent.start_time_ms = 5000; // 2000ms pause between distinct sentences
    seg_new_sent.end_time_ms = 7000;
    seg_new_sent.text = "Por otro lado, continuamos.";

    EXPECT_FALSE(TranscriptSentenceStitcher::is_continuation(seg_period, seg_new_sent, 5));

    // Period with Whisper false-period breath pause + lowercase continuation -> true
    TranscriptSegment seg_false_dot;
    seg_false_dot.start_time_ms = 0;
    seg_false_dot.end_time_ms = 4000;
    seg_false_dot.text = "la inteligencia artificial, la biotecnología.";

    TranscriptSegment seg_lower;
    seg_lower.start_time_ms = 4400; // 400ms breath gap
    seg_lower.end_time_ms = 8000;
    seg_lower.text = "el progreso tecnológico ha modificado profundamente.";

    EXPECT_TRUE(TranscriptSentenceStitcher::is_continuation(seg_false_dot, seg_lower, 5));

    // Edge cases: empty texts or max words exceeded
    TranscriptSegment empty_seg;
    EXPECT_FALSE(TranscriptSentenceStitcher::is_continuation(empty_seg, seg_cont, 0));
    EXPECT_FALSE(TranscriptSentenceStitcher::is_continuation(seg_comma, empty_seg, 0));
    EXPECT_FALSE(TranscriptSentenceStitcher::is_continuation(seg_comma, seg_cont, 60)); // max limit 55
}

TEST(TranscriptSentenceStitcherTest, StitchCompoundSentence) {
    TranscriptSegment s1;
    s1.start_time_ms = 0;
    s1.end_time_ms = 4200;
    s1.text = "Desde los avances en la informática y las comunicaciones hasta el desarrollo de la inteligencia artificial, la biotecnología.";
    s1.confidence_score = 0.90f;
    s1.avg_logprob = -0.15f;

    TranscriptSegment s2;
    s2.start_time_ms = 4650; // 450ms gap
    s2.end_time_ms = 9500;
    s2.text = "el progreso tecnológico ha modificado profundamente la manera en que las personas trabajan, se comunican y aprenden.";
    s2.confidence_score = 0.85f;
    s2.avg_logprob = -0.22f;

    TranscriptSegment s3;
    s3.start_time_ms = 12000; // 2500ms gap -> new independent sentence
    s3.end_time_ms = 15000;
    s3.text = "En segundo lugar abordaremos la metodología.";
    s3.confidence_score = 0.95f;
    s3.avg_logprob = -0.10f;

    std::vector<TranscriptSegment> raw = {s1, s2, s3};
    auto stitched = TranscriptSentenceStitcher::stitch_segments(raw);

    ASSERT_EQ(stitched.size(), 2);

    // First stitched sentence should combine s1 + s2
    EXPECT_EQ(stitched[0].start_time_ms, 0);
    EXPECT_EQ(stitched[0].end_time_ms, 9500);
    EXPECT_NE(stitched[0].text.find("biotecnología el progreso tecnológico"), std::string::npos);
    EXPECT_GT(stitched[0].confidence_score, 0.80f);

    // Second sentence remains untouched
    EXPECT_EQ(stitched[1].start_time_ms, 12000);
    EXPECT_EQ(stitched[1].end_time_ms, 15000);
    EXPECT_EQ(stitched[1].text, "En segundo lugar abordaremos la metodología.");
}

TEST(TranscriptSentenceStitcherTest, EmptyAndSingleSegment) {
    std::vector<TranscriptSegment> empty_list;
    auto res_empty = TranscriptSentenceStitcher::stitch_segments(empty_list);
    EXPECT_TRUE(res_empty.empty());

    TranscriptSegment single;
    single.start_time_ms = 100;
    single.end_time_ms = 2000;
    single.text = "Solo una frase corta.";
    single.confidence_score = 0.90f;

    auto res_single = TranscriptSentenceStitcher::stitch_segments({single});
    ASSERT_EQ(res_single.size(), 1);
    EXPECT_EQ(res_single[0].text, "Solo una frase corta.");
    EXPECT_EQ(res_single[0].start_time_ms, 100);
    EXPECT_EQ(res_single[0].end_time_ms, 2000);
    ASSERT_EQ(res_single[0].source_segment_indices.size(), 1);
    EXPECT_EQ(res_single[0].source_segment_indices[0], 0);
}

TEST(TranscriptSentenceStitcherTest, CapitalizedContinuationWordAndSourceIndices) {
    // Segment 1 (Index 0): Ends with period, followed by 480ms gap
    TranscriptSegment s1;
    s1.start_time_ms = 8000;
    s1.end_time_ms = 15320;
    s1.text = "Desde los avances en la informática y las comunicaciones hasta el desarrollo de la inteligencia artificial, la biotecnología.";
    s1.confidence_score = 0.95f;

    // Segment 2 (Index 1): Starts with Capitalized "El" after false period
    TranscriptSegment s2;
    s2.start_time_ms = 15800; // 480ms gap
    s2.end_time_ms = 23540;
    s2.text = "El proceso tecnológico ha modificado profundamente la manera en que las personas trabajan.";
    s2.confidence_score = 0.98f;

    // Segment 3 (Index 2): Next topic after long pause
    TranscriptSegment s3;
    s3.start_time_ms = 26000; // 2460ms gap
    s3.end_time_ms = 35000;
    s3.text = "Lejos de limitarse a dispositivos electrónicos.";
    s3.confidence_score = 0.99f;

    std::vector<TranscriptSegment> raw = {s1, s2, s3};
    auto stitched = TranscriptSentenceStitcher::stitch_segments(raw);

    ASSERT_EQ(stitched.size(), 2);

    // First stitched sentence combines s1 and s2
    EXPECT_EQ(stitched[0].start_time_ms, 8000);
    EXPECT_EQ(stitched[0].end_time_ms, 23540);
    ASSERT_EQ(stitched[0].source_segment_indices.size(), 2);
    EXPECT_EQ(stitched[0].source_segment_indices[0], 0);
    EXPECT_EQ(stitched[0].source_segment_indices[1], 1);

    // Second sentence is s3
    EXPECT_EQ(stitched[1].start_time_ms, 26000);
    EXPECT_EQ(stitched[1].end_time_ms, 35000);
    ASSERT_EQ(stitched[1].source_segment_indices.size(), 1);
    EXPECT_EQ(stitched[1].source_segment_indices[0], 2);
}
