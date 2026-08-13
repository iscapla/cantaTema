/**
 * @file t_ctc_aligner.cxx
 * @brief Unit tests for ICtcAligner, WhisperCtcAligner, and CtcAlignmentManager.
 */

#include <gtest/gtest.h>
#include "speech_recognition/whisper_ctc_aligner.hpp"
#include "speech_recognition/ctc_alignment_manager.hpp"
#include "configuration/configuration_system.hpp"

TEST(TestCtcAligner, WhisperCtcAlignerBasicAlignment) {
    WhisperCtcAligner aligner;
    EXPECT_EQ(aligner.get_aligner_id(), "whisper_ctc");

    std::vector<float> empty_samples;
    std::vector<std::string> empty_tokens;
    auto res_empty = aligner.align_tokens(empty_samples, 16000, empty_tokens);
    EXPECT_TRUE(res_empty.empty());

    // Generate 1 second of synthetic PCM audio (16000 samples)
    std::vector<float> pcm_samples(16000, 0.1f);
    std::vector<std::string> tokens = {"hello", "world", "test"};

    auto aligned = aligner.align_tokens(pcm_samples, 16000, tokens);
    ASSERT_EQ(aligned.size(), 3u);

    EXPECT_EQ(aligned[0].word, "hello");
    EXPECT_GE(aligned[0].start_ms, 0);
    EXPECT_LE(aligned[0].end_ms, 1000);

    EXPECT_EQ(aligned[1].word, "world");
    EXPECT_GE(aligned[1].start_ms, aligned[0].start_ms);

    EXPECT_EQ(aligned[2].word, "test");
    EXPECT_GE(aligned[2].start_ms, aligned[1].start_ms);
}

TEST(TestCtcAligner, CtcAlignmentManagerRetrieval) {
    auto& manager = CtcAlignmentManager::getInstance();

    auto whisper_ctc = manager.get_aligner("whisper_ctc");
    ASSERT_NE(whisper_ctc, nullptr);
    EXPECT_EQ(whisper_ctc->get_aligner_id(), "whisper_ctc");

    // Fallback to default on unknown aligner
    auto unknown = manager.get_aligner("wav2vec2_unknown");
    ASSERT_NE(unknown, nullptr);
    EXPECT_EQ(unknown->get_aligner_id(), "whisper_ctc");

    // Active aligner from configuration
    ConfigurationSystem::getInstance().set_value("ALIGNMENT", "alignment_mode", "whisper_ctc");
    auto active = manager.get_active_aligner();
    ASSERT_NE(active, nullptr);
    EXPECT_EQ(active->get_aligner_id(), "whisper_ctc");
}

TEST(TestCtcAligner, ManagerRegisterNull) {
    auto& manager = CtcAlignmentManager::getInstance();
    manager.register_aligner(nullptr);
    EXPECT_NE(manager.get_aligner("whisper_ctc"), nullptr);
}
