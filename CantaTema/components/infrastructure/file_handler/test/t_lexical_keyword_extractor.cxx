/**
 * @file t_lexical_keyword_extractor.cxx
 * @brief Unit tests for LexicalKeywordExtractor class.
 */

#include <gtest/gtest.h>
#include "file_handler/lexical_keyword_extractor.hpp"

TEST(LexicalKeywordExtractorTest, CountsWordsAccurately) {
    EXPECT_EQ(LexicalKeywordExtractor::count_words("Naturaleza del Impuesto."), 3u);
    EXPECT_EQ(LexicalKeywordExtractor::count_words("cuando el contribuyente hubiera renunciado a su aplicación."), 8u);
    EXPECT_EQ(LexicalKeywordExtractor::count_words(""), 0u);
}

TEST(LexicalKeywordExtractorTest, ExtractsContentKeywordsFilteringStopWords) {
    std::string text = "Naturaleza del Impuesto sobre la renta";
    auto keywords = LexicalKeywordExtractor::extract_keywords(text);

    EXPECT_TRUE(keywords.count("naturaleza") > 0);
    EXPECT_TRUE(keywords.count("impuesto") > 0);
    EXPECT_TRUE(keywords.count("renta") > 0);

    // Stop-words should be filtered
    EXPECT_EQ(keywords.count("del"), 0u);
    EXPECT_EQ(keywords.count("sobre"), 0u);
    EXPECT_EQ(keywords.count("la"), 0u);
}

TEST(LexicalKeywordExtractorTest, ComputesKeywordOverlap) {
    std::string heading = "Naturaleza del Impuesto.";
    std::string trans_unrelated = "cuando el contribuyente hubiera renunciado a su aplicación.";
    std::string trans_related = "Explica la naturaleza jurídica del impuesto directo.";

    auto head_kw = LexicalKeywordExtractor::extract_keywords(heading);
    auto unrel_kw = LexicalKeywordExtractor::extract_keywords(trans_unrelated);
    auto rel_kw = LexicalKeywordExtractor::extract_keywords(trans_related);

    EXPECT_EQ(LexicalKeywordExtractor::count_keyword_overlap(head_kw, unrel_kw), 0u);
    EXPECT_GT(LexicalKeywordExtractor::count_keyword_overlap(head_kw, rel_kw), 0u);
}
