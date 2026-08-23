#include <gtest/gtest.h>
#include "similarity/word_sequence_aligner.hpp"

TEST(WordSequenceAlignerTest, NormalizeWordStripsAccentsAndPunctuation) {
    EXPECT_EQ(WordSequenceAligner::normalize_word("Página."), "pagina");
    EXPECT_EQ(WordSequenceAligner::normalize_word("¿Tecnología!"), "tecnologia");
    EXPECT_EQ(WordSequenceAligner::normalize_word("artículo;"), "articulo");
    EXPECT_EQ(WordSequenceAligner::normalize_word("15,00"), "1500");
}

TEST(WordSequenceAlignerTest, TokenizeSplitsWhitespace) {
    auto tokens = WordSequenceAligner::tokenize("Esta es una prueba de texto.");
    ASSERT_EQ(tokens.size(), 6u);
    EXPECT_EQ(tokens[0], "Esta");
    EXPECT_EQ(tokens[5], "texto.");
}

TEST(WordSequenceAlignerTest, ExactMatch100Percent) {
    std::string ref = "El impuesto sobre la renta de las personas físicas es un tributo directo.";
    std::string trans = "El impuesto sobre la renta de las personas físicas es un tributo directo.";

    auto res = WordSequenceAligner::align(ref, trans);
    EXPECT_EQ(res.total_reference_words, 13u);
    EXPECT_EQ(res.matched_word_count, 13u);
    EXPECT_EQ(res.omitted_word_count, 0u);
    EXPECT_FLOAT_EQ(res.word_recall_score, 1.0f);
}

TEST(WordSequenceAlignerTest, OmittedWordsDetection) {
    std::string ref = "El impuesto sobre la renta es de carácter personal y directo.";
    std::string trans = "El impuesto sobre la renta es directo.";

    auto res = WordSequenceAligner::align(ref, trans);
    EXPECT_EQ(res.total_reference_words, 11u);
    EXPECT_EQ(res.matched_word_count, 7u);
    EXPECT_EQ(res.omitted_word_count, 4u);
    EXPECT_FLOAT_EQ(res.word_recall_score, 7.0f / 11.0f);

    // Verify "carácter" and "personal" are marked OMITTED
    EXPECT_EQ(res.reference_words[7].status, WordDiffStatus::OMITTED); // de
    EXPECT_EQ(res.reference_words[8].status, WordDiffStatus::OMITTED); // carácter
    EXPECT_EQ(res.reference_words[9].status, WordDiffStatus::OMITTED); // personal
}

TEST(WordSequenceAlignerTest, WeightedRecallAndLegalCitationTracking) {
    std::string ref = "Artículo 7. Estarán exentas las rentas por el 50% de deducción.";
    std::string trans = "Estarán exentas las rentas por el 50% de deducción.";

    auto res = WordSequenceAligner::align(ref, trans);
    // "Artículo 7." omitted!
    EXPECT_TRUE(res.has_missing_legal_citation);
    EXPECT_TRUE(res.reference_words[0].is_legal_citation); // Artículo
    EXPECT_TRUE(res.reference_words[1].is_legal_citation); // 7.
    EXPECT_GT(res.reference_words[0].weight, 3.0f); // 4.0x
    EXPECT_LT(res.reference_words[4].weight, 0.5f); // "las" stopword at index 4 (0.2x)

    EXPECT_GT(res.weighted_recall_score, 0.0f);
    EXPECT_LT(res.weighted_recall_score, 1.0f);
}

TEST(WordSequenceAlignerTest, SemanticSynonymWordAlignment) {
    std::string ref = "La ley establece el requisito obligatorio.";
    std::string trans = "La ley fija el presupuesto preceptivo.";

    auto res = WordSequenceAligner::align(ref, trans, "law", "es");
    EXPECT_EQ(res.total_reference_words, 6u);
    EXPECT_EQ(res.matched_word_count, 6u); // all 6 accounted for (3 exact, 3 semantic)
    EXPECT_EQ(res.semantic_word_count, 3u); // establece->fija, requisito->presupuesto, obligatorio->preceptivo
    EXPECT_EQ(res.omitted_word_count, 0u);

    // Verify token states
    EXPECT_EQ(res.reference_words[0].status, WordDiffStatus::MATCHED); // La
    EXPECT_EQ(res.reference_words[1].status, WordDiffStatus::MATCHED); // ley
    EXPECT_EQ(res.reference_words[2].status, WordDiffStatus::SEMANTIC_EQUIVALENCE); // establece -> fija
    EXPECT_EQ(res.reference_words[3].status, WordDiffStatus::MATCHED); // el
    EXPECT_EQ(res.reference_words[4].status, WordDiffStatus::SEMANTIC_EQUIVALENCE); // requisito -> presupuesto
    EXPECT_EQ(res.reference_words[5].status, WordDiffStatus::SEMANTIC_EQUIVALENCE); // obligatorio -> preceptivo
}

TEST(WordSequenceAlignerTest, MultiWordDomainParaphraseAlignment) {
    std::string ref = "El hecho imponible genera la obligación tributaria.";
    std::string trans = "El presupuesto de hecho origina la obligación tributaria.";

    auto res = WordSequenceAligner::align(ref, trans, "law", "es");
    EXPECT_EQ(res.total_reference_words, 7u);
    EXPECT_GE(res.matched_word_count, 6u);
    EXPECT_GE(res.semantic_word_count, 2u); // hecho, imponible

    EXPECT_EQ(res.reference_words[1].status, WordDiffStatus::SEMANTIC_EQUIVALENCE); // hecho
    EXPECT_EQ(res.reference_words[2].status, WordDiffStatus::SEMANTIC_EQUIVALENCE); // imponible
}

