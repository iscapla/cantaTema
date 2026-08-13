/**
 * @file t_phonetic_matcher.cxx
 * @brief Unit tests for IPhoneticMatcher, DoubleMetaphoneMatcher, SoundexMatcher, PhoneticMatcherManager, and WordSequenceAligner integration.
 */

#include <gtest/gtest.h>
#include "similarity/double_metaphone_matcher.hpp"
#include "similarity/soundex_matcher.hpp"
#include "similarity/phonetic_matcher_manager.hpp"
#include "similarity/word_sequence_aligner.hpp"
#include "configuration/configuration_system.hpp"

TEST(TestPhoneticMatcher, DoubleMetaphoneMatcherEquivalence) {
    DoubleMetaphoneMatcher matcher;
    EXPECT_EQ(matcher.get_matcher_id(), "double_metaphone");

    auto res1 = matcher.compare_words("extinción", "estincion");
    EXPECT_TRUE(res1.is_match);
    EXPECT_TRUE(res1.is_minor_mispronunciation);
    EXPECT_FLOAT_EQ(res1.similarity_score, 0.85f);

    auto res2 = matcher.compare_words("excepción", "escepcion");
    EXPECT_TRUE(res2.is_match);
    EXPECT_TRUE(res2.is_minor_mispronunciation);

    auto res3 = matcher.compare_words("constitucion", "ley");
    EXPECT_FALSE(res3.is_match);
    EXPECT_FLOAT_EQ(res3.similarity_score, 0.0f);
}

TEST(TestPhoneticMatcher, SoundexMatcherEquivalence) {
    SoundexMatcher matcher;
    EXPECT_EQ(matcher.get_matcher_id(), "soundex");

    auto res1 = matcher.compare_words("Smith", "Smyth");
    EXPECT_TRUE(res1.is_match);
    EXPECT_TRUE(res1.is_minor_mispronunciation);

    auto res2 = matcher.compare_words("Robert", "Rupert");
    EXPECT_TRUE(res2.is_match);

    auto res3 = matcher.compare_words("cat", "dog");
    EXPECT_FALSE(res3.is_match);
}

TEST(TestPhoneticMatcher, PhoneticMatcherManagerRetrieval) {
    auto& manager = PhoneticMatcherManager::getInstance();

    auto dm = manager.get_matcher("double_metaphone");
    ASSERT_NE(dm, nullptr);
    EXPECT_EQ(dm->get_matcher_id(), "double_metaphone");

    auto sx = manager.get_matcher("soundex");
    ASSERT_NE(sx, nullptr);
    EXPECT_EQ(sx->get_matcher_id(), "soundex");

    // Unknown falls back to default double_metaphone
    auto unknown = manager.get_matcher("unknown_algorithm");
    ASSERT_NE(unknown, nullptr);
    EXPECT_EQ(unknown->get_matcher_id(), "double_metaphone");

    // Active matcher from configuration
    ConfigurationSystem::getInstance().set_value("PHONETIC", "default_matcher", "soundex");
    auto active = manager.get_active_matcher();
    ASSERT_NE(active, nullptr);
    EXPECT_EQ(active->get_matcher_id(), "soundex");

    // Reset back to double_metaphone
    ConfigurationSystem::getInstance().set_value("PHONETIC", "default_matcher", "double_metaphone");
}

TEST(TestPhoneticMatcher, WordSequenceAlignerPhoneticIntegration) {
    ConfigurationSystem::getInstance().set_value("PHONETIC", "enable_phonetic_matching", "true");
    ConfigurationSystem::getInstance().set_value("PHONETIC", "default_matcher", "double_metaphone");

    std::string ref = "extinción de la obligación";
    std::string trans = "estincion de la obligacion";

    WordAlignmentResult result = WordSequenceAligner::align(ref, trans);
    ASSERT_FALSE(result.reference_words.empty());

    // First word "extinción" misheard as "estincion" should be PHONETIC_MISPRONUNCIATION
    EXPECT_EQ(result.reference_words[0].status, WordDiffStatus::PHONETIC_MISPRONUNCIATION);
}
