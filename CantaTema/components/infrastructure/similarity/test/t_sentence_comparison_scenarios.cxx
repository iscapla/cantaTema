/**
 * @file t_sentence_comparison_scenarios.cxx
 * @brief Comprehensive test suite evaluating 1-vs-1 sentence comparison scenarios across all diff statuses and edge cases.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "similarity/word_sequence_aligner.hpp"
#include "similarity/dictionary_paraphrase_matcher.hpp"
#include "similarity/embedding_paraphrase_matcher.hpp"
#include "similarity/hybrid_paraphrase_matcher.hpp"
#include "similarity/paraphrase_matcher_manager.hpp"
#include "similarity/phonetic_matcher_manager.hpp"
#include "configuration/configuration_system.hpp"

class SentenceComparisonTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Ensure default standard configuration is active
        ConfigurationSystem::getInstance().set_value("PHONETIC", "enable_phonetic_matching", "true");
        ConfigurationSystem::getInstance().set_value("PHONETIC", "default_matcher", "double_metaphone");
        ConfigurationSystem::getInstance().set_value("SEMANTIC_PARAPHRASE", "enable_paraphrasing", "true");
        ConfigurationSystem::getInstance().set_value("SEMANTIC_PARAPHRASE", "mode", "hybrid");
        ConfigurationSystem::getInstance().set_value("SEMANTIC_PARAPHRASE", "default_matcher", "hybrid");
        ConfigurationSystem::getInstance().set_value("SEMANTIC_PARAPHRASE", "semantic_weight_credit", "0.95");
    }
};

// Scenario 1: Exact Sentence Match (100% Identical Recall, Punctuation/Accent Invariant)
TEST_F(SentenceComparisonTest, Scenario1_ExactMatch_100PercentRecall) {
    std::string ref = "El recurso de alzada se interpondrá ante el órgano superior jerárquico.";
    std::string trans = "el recurso de alzada se interpondra ante el organo superior jerarquico.";

    auto res = WordSequenceAligner::align(ref, trans, "law", "es");

    EXPECT_EQ(res.total_reference_words, 11u);
    EXPECT_EQ(res.matched_word_count, 11u);
    EXPECT_EQ(res.omitted_word_count, 0u);
    EXPECT_EQ(res.phonetic_word_count, 0u);
    EXPECT_EQ(res.semantic_word_count, 0u);
    EXPECT_FLOAT_EQ(res.word_recall_score, 1.0f);
    EXPECT_FLOAT_EQ(res.weighted_recall_score, 1.0f);
    EXPECT_FALSE(res.has_missing_legal_citation);

    for (const auto& token : res.reference_words) {
        EXPECT_EQ(token.status, WordDiffStatus::MATCHED);
    }
}

// Scenario 2: Sentence with Word Omissions (Partial Speech)
TEST_F(SentenceComparisonTest, Scenario2_OmittedWords_PartialSpeech) {
    std::string ref = "El plazo máximo para dictar y notificar la resolución será de seis meses.";
    std::string trans = "El plazo para dictar la resolución será de seis meses.";

    auto res = WordSequenceAligner::align(ref, trans, "law", "es");

    EXPECT_EQ(res.total_reference_words, 13u);
    EXPECT_EQ(res.matched_word_count, 10u);
    EXPECT_EQ(res.omitted_word_count, 3u); // "máximo", "y", "notificar"
    EXPECT_FLOAT_EQ(res.word_recall_score, 10.0f / 13.0f);
    EXPECT_LT(res.weighted_recall_score, 1.0f);
    EXPECT_GT(res.weighted_recall_score, 0.5f);

    // Verify specific omitted words
    EXPECT_EQ(res.reference_words[2].normalized_word, "maximo");
    EXPECT_EQ(res.reference_words[2].status, WordDiffStatus::OMITTED);

    EXPECT_EQ(res.reference_words[5].normalized_word, "y");
    EXPECT_EQ(res.reference_words[5].status, WordDiffStatus::OMITTED);

    EXPECT_EQ(res.reference_words[6].normalized_word, "notificar");
    EXPECT_EQ(res.reference_words[6].status, WordDiffStatus::OMITTED);
}

// Scenario 3: Sentence with Phonetic Mispronunciations / Speech Glitches
TEST_F(SentenceComparisonTest, Scenario3_PhoneticMispronunciations_SpeechGlitches) {
    std::string ref = "La extinción de la excepción tributaria extingue la obligación.";
    std::string trans = "La estincion de la escepcion tributaria extingue la obligacion.";

    auto res = WordSequenceAligner::align(ref, trans, "law", "es");

    EXPECT_EQ(res.total_reference_words, 9u);
    EXPECT_EQ(res.matched_word_count, 9u); // All accounted for via exact or phonetic
    EXPECT_GE(res.phonetic_word_count, 2u); // "extinción" -> "estincion", "excepción" -> "escepcion"
    EXPECT_EQ(res.omitted_word_count, 0u);
    EXPECT_FLOAT_EQ(res.word_recall_score, 1.0f);
    // Weighted recall receives 85% partial credit for phonetic tokens
    EXPECT_LT(res.weighted_recall_score, 1.0f);
    EXPECT_GT(res.weighted_recall_score, 0.85f);
}

// Scenario 4: Sentence with Single-Word Synonyms & Inflected Verbs
TEST_F(SentenceComparisonTest, Scenario4_SingleWordSynonyms_InflectedVerbs) {
    std::string ref = "La administración pública debe notificar y motivar todos sus actos reglamentarios.";
    std::string trans = "La administración pública debe comunicar y justificar todos sus actos reglamentarios.";

    auto res = WordSequenceAligner::align(ref, trans, "law", "es");

    EXPECT_EQ(res.total_reference_words, 11u);
    EXPECT_EQ(res.matched_word_count, 11u);
    EXPECT_GE(res.semantic_word_count, 2u); // "notificar" -> "comunicar", "motivar" -> "justificar"
    EXPECT_EQ(res.omitted_word_count, 0u);

    // Verify token statuses for synonyms
    EXPECT_EQ(res.reference_words[4].normalized_word, "notificar");
    EXPECT_EQ(res.reference_words[4].status, WordDiffStatus::SEMANTIC_EQUIVALENCE);
    EXPECT_EQ(res.reference_words[6].normalized_word, "motivar");
    EXPECT_EQ(res.reference_words[6].status, WordDiffStatus::SEMANTIC_EQUIVALENCE);
}

// Scenario 5: Sentence with Multi-Word Domain Idioms across Law, Economics, and Science
TEST_F(SentenceComparisonTest, Scenario5_MultiWordDomainIdioms_Law_Economics_Science) {
    // 1. Law Domain Multi-Word Phrase
    {
        std::string ref_law = "El hecho imponible genera la obligación tributaria principal.";
        std::string trans_law = "El presupuesto de hecho origina la obligación tributaria principal.";

        auto res_law = WordSequenceAligner::align(ref_law, trans_law, "law", "es");
        EXPECT_EQ(res_law.total_reference_words, 8u);
        EXPECT_GE(res_law.semantic_word_count, 2u); // "hecho imponible" -> "presupuesto de hecho"
        EXPECT_EQ(res_law.reference_words[1].status, WordDiffStatus::SEMANTIC_EQUIVALENCE);
        EXPECT_EQ(res_law.reference_words[2].status, WordDiffStatus::SEMANTIC_EQUIVALENCE);
    }

    // 2. Economics Domain Multi-Word Phrase
    {
        std::string ref_econ = "El tipo de interés fija el coste de financiación empresarial.";
        std::string trans_econ = "El precio del dinero fija el coste de financiación empresarial.";

        auto res_econ = WordSequenceAligner::align(ref_econ, trans_econ, "economics", "es");
        EXPECT_GE(res_econ.semantic_word_count, 3u); // "tipo de interes" -> "precio del dinero"
        EXPECT_EQ(res_econ.reference_words[1].status, WordDiffStatus::SEMANTIC_EQUIVALENCE);
        EXPECT_EQ(res_econ.reference_words[2].status, WordDiffStatus::SEMANTIC_EQUIVALENCE);
        EXPECT_EQ(res_econ.reference_words[3].status, WordDiffStatus::SEMANTIC_EQUIVALENCE);
    }

    // 3. Science Domain Multi-Word Phrase
    {
        std::string ref_sci = "El material genético se almacena en el núcleo celular.";
        std::string trans_sci = "El acido desoxirribonucleico se almacena en el núcleo celular.";

        auto res_sci = WordSequenceAligner::align(ref_sci, trans_sci, "science", "es");
        EXPECT_GE(res_sci.semantic_word_count, 2u); // "material genetico"
        EXPECT_EQ(res_sci.reference_words[1].status, WordDiffStatus::SEMANTIC_EQUIVALENCE);
        EXPECT_EQ(res_sci.reference_words[2].status, WordDiffStatus::SEMANTIC_EQUIVALENCE);
    }
}

// Scenario 6: Sentence with Missing Critical Legal Citations (Articles / Law Numbers)
TEST_F(SentenceComparisonTest, Scenario6_MissingLegalCitation_ArticleAndLawNumber) {
    std::string ref = "Conforme al Artículo 14 de la Ley 39/2015 se establece el derecho de audiencia.";
    std::string trans = "Se establece el derecho de audiencia en el procedimiento.";

    auto res = WordSequenceAligner::align(ref, trans, "law", "es");

    EXPECT_TRUE(res.has_missing_legal_citation);

    // Verify legal tokens are recognized with high weight and flagged omitted
    EXPECT_TRUE(res.reference_words[2].is_legal_citation); // Artículo
    EXPECT_EQ(res.reference_words[2].status, WordDiffStatus::OMITTED);
    EXPECT_FLOAT_EQ(res.reference_words[2].weight, 4.0f);

    EXPECT_TRUE(res.reference_words[3].is_legal_citation); // 14 (enumerator / legal citation number)
    EXPECT_EQ(res.reference_words[3].status, WordDiffStatus::OMITTED);
    EXPECT_FLOAT_EQ(res.reference_words[3].weight, 4.0f);

    EXPECT_TRUE(res.reference_words[6].is_legal_citation); // Ley
    EXPECT_EQ(res.reference_words[6].status, WordDiffStatus::OMITTED);
    EXPECT_FLOAT_EQ(res.reference_words[6].weight, 4.0f);

    EXPECT_TRUE(res.reference_words[7].is_numeric); // 39/2015
    EXPECT_EQ(res.reference_words[7].status, WordDiffStatus::OMITTED);

    // Heavily penalizes weighted recall due to high citation weights
    EXPECT_LT(res.weighted_recall_score, 0.60f);
}

// Scenario 7: Sentence with Numeric / Quantity Discrepancy
TEST_F(SentenceComparisonTest, Scenario7_NumericDiscrepancy_Warning) {
    std::string ref = "La deducción fiscal aplicable será del 25% sobre la cuota íntegra.";
    std::string trans = "La deducción fiscal aplicable será del 50% sobre la cuota íntegra.";

    auto res = WordSequenceAligner::align(ref, trans, "law", "es");

    EXPECT_EQ(res.reference_words[6].normalized_word, "25%");
    EXPECT_TRUE(res.reference_words[6].is_numeric);
    EXPECT_FLOAT_EQ(res.reference_words[6].weight, 3.0f);
    EXPECT_EQ(res.reference_words[6].status, WordDiffStatus::OMITTED); // 25% not matched with 50%

    // Domain safety guards prevent numeric mismatch from being counted as a semantic paraphrase
    EXPECT_FALSE(EmbeddingParaphraseMatcher::passes_safety_guards("25%", "50%"));
    EXPECT_FALSE(EmbeddingParaphraseMatcher::passes_safety_guards("plazo de 15 dias", "plazo de 30 dias"));
    EXPECT_TRUE(EmbeddingParaphraseMatcher::passes_safety_guards("plazo de 15 dias", "termino de 15 dias"));
}

// Scenario 8: Sentence with Negation Inversion (Direct Contradiction)
TEST_F(SentenceComparisonTest, Scenario8_NegationInversion_SafetyCheck) {
    std::string ref = "La interposición del recurso no suspende la ejecución del acto impugnado.";
    std::string trans = "La interposición del recurso suspende la ejecución del acto impugnado.";

    auto res = WordSequenceAligner::align(ref, trans, "law", "es");

    // The critical negation word "no" must be marked OMITTED
    EXPECT_EQ(res.reference_words[4].normalized_word, "no");
    EXPECT_EQ(res.reference_words[4].status, WordDiffStatus::OMITTED);

    // Safety guards detect contradiction
    EXPECT_FALSE(EmbeddingParaphraseMatcher::passes_safety_guards("no suspende la ejecucion", "suspende la ejecucion"));
    EXPECT_FALSE(EmbeddingParaphraseMatcher::passes_safety_guards("esta prohibido", "nunca esta prohibido"));
}

// Scenario 9: Sentence with Extra Transcript Fillers / Expansions (Spurious Words)
TEST_F(SentenceComparisonTest, Scenario9_ExtraTranscriptFillers_SpuriousWordsIgnored) {
    std::string ref = "Los actos administrativos son inmediatamente ejecutivos.";
    std::string trans = "Bueno, eh, básicamente los actos administrativos son inmediatamente ejecutivos y eficaces en todo caso.";

    auto res = WordSequenceAligner::align(ref, trans, "law", "es");

    // All reference words are 100% matched, extra spoken fillers don't penalize reference recall
    EXPECT_EQ(res.total_reference_words, 6u);
    EXPECT_EQ(res.matched_word_count, 6u);
    EXPECT_EQ(res.omitted_word_count, 0u);
    EXPECT_FLOAT_EQ(res.word_recall_score, 1.0f);
    EXPECT_FLOAT_EQ(res.weighted_recall_score, 1.0f);
}

// Scenario 10: Sentence with Reordered Clauses (Inverted Syntax)
TEST_F(SentenceComparisonTest, Scenario10_ReorderedClauses_InvertedSyntax) {
    std::string ref = "El procedimiento caducará si se produce la paralización por causa imputable al interesado.";
    std::string trans = "Si se produce la paralización por causa imputable al interesado el procedimiento caducará.";

    auto res = WordSequenceAligner::align(ref, trans, "law", "es");

    // Longest common subsequence matches the longest contiguous block
    EXPECT_EQ(res.total_reference_words, 13u);
    EXPECT_GE(res.matched_word_count, 10u);
    EXPECT_GT(res.word_recall_score, 0.75f);
}

// Scenario 11: Multilingual 1-vs-1 English Sentence Comparison
TEST_F(SentenceComparisonTest, Scenario11_EnglishMultilingualComparison) {
    std::string ref = "The statute establishes mandatory requirements for tax liabilities.";
    std::string trans = "The law determines compulsory conditions for tax liabilities.";

    auto res = WordSequenceAligner::align(ref, trans, "general", "en");

    EXPECT_EQ(res.total_reference_words, 8u);
    EXPECT_EQ(res.matched_word_count, 8u); // All 8 words accounted for
    EXPECT_GE(res.semantic_word_count, 3u); // establishes->determines, mandatory->compulsory, requirements->conditions
    EXPECT_EQ(res.omitted_word_count, 0u);

    EXPECT_EQ(res.reference_words[1].status, WordDiffStatus::SEMANTIC_EQUIVALENCE); // establishes
    EXPECT_EQ(res.reference_words[2].status, WordDiffStatus::SEMANTIC_EQUIVALENCE); // mandatory
    EXPECT_EQ(res.reference_words[3].status, WordDiffStatus::SEMANTIC_EQUIVALENCE); // requirements
}

// Scenario 12: Completely Unrelated / Irrelevant Sentence (0% Recall)
TEST_F(SentenceComparisonTest, Scenario12_CompletelyUnrelatedSentence_ZeroRecall) {
    std::string ref = "El tribunal constitucional es el intérprete supremo de la constitución.";
    std::string trans = "Hoy hace un día muy soleado y vamos a ir a la playa a descansar.";

    auto res = WordSequenceAligner::align(ref, trans, "law", "es");

    // All content reference words are marked OMITTED
    EXPECT_EQ(res.reference_words[1].normalized_word, "tribunal");
    EXPECT_EQ(res.reference_words[1].status, WordDiffStatus::OMITTED);

    EXPECT_EQ(res.reference_words[2].normalized_word, "constitucional");
    EXPECT_EQ(res.reference_words[2].status, WordDiffStatus::OMITTED);

    EXPECT_EQ(res.reference_words[5].normalized_word, "interprete");
    EXPECT_EQ(res.reference_words[5].status, WordDiffStatus::OMITTED);

    EXPECT_EQ(res.reference_words[6].normalized_word, "supremo");
    EXPECT_EQ(res.reference_words[6].status, WordDiffStatus::OMITTED);

    EXPECT_EQ(res.reference_words[9].normalized_word, "constitucion");
    EXPECT_EQ(res.reference_words[9].status, WordDiffStatus::OMITTED);

    // Recall of content words is extremely low / negligible
    EXPECT_LT(res.weighted_recall_score, 0.20f);
}

// Scenario 13: Empty and Whitespace Edge Cases (Zero-Division Safe)
TEST_F(SentenceComparisonTest, Scenario13_EmptyAndWhitespaceEdgeCases) {
    // Both empty
    auto res1 = WordSequenceAligner::align("", "");
    EXPECT_EQ(res1.total_reference_words, 0u);
    EXPECT_EQ(res1.matched_word_count, 0u);
    EXPECT_FLOAT_EQ(res1.word_recall_score, 0.0f);
    EXPECT_FLOAT_EQ(res1.weighted_recall_score, 0.0f);

    // Empty transcript
    auto res2 = WordSequenceAligner::align("Texto de referencia válido.", "");
    EXPECT_EQ(res2.total_reference_words, 4u);
    EXPECT_EQ(res2.matched_word_count, 0u);
    EXPECT_EQ(res2.omitted_word_count, 4u);
    EXPECT_FLOAT_EQ(res2.word_recall_score, 0.0f);

    // Empty reference
    auto res3 = WordSequenceAligner::align("", "Transcripción hablada por el estudiante.");
    EXPECT_EQ(res3.total_reference_words, 0u);
    EXPECT_FLOAT_EQ(res3.word_recall_score, 0.0f);

    // Whitespace and punctuation only
    auto res4 = WordSequenceAligner::align("   ... ,,, !!!   ", "\t\t\n\n");
    EXPECT_FLOAT_EQ(res4.word_recall_score, 0.0f);
    EXPECT_FLOAT_EQ(res4.weighted_recall_score, 0.0f);
}
