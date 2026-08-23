/**
 * @file t_paraphrase_matcher.cxx
 * @brief Unit tests for IParaphraseMatcher, DictionaryParaphraseMatcher, EmbeddingParaphraseMatcher, HybridParaphraseMatcher, and ParaphraseMatcherManager.
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "similarity/dictionary_paraphrase_matcher.hpp"
#include "similarity/embedding_paraphrase_matcher.hpp"
#include "similarity/hybrid_paraphrase_matcher.hpp"
#include "similarity/paraphrase_matcher_manager.hpp"
#include "similarity/word_sequence_aligner.hpp"
#include "embeddings/mocks/mock_embedding_engine.hpp"
#include "configuration/configuration_system.hpp"

using ::testing::Return;
using ::testing::_;

TEST(TestParaphraseMatcher, DictionarySpanishSynonyms) {
    DictionaryParaphraseMatcher matcher;
    EXPECT_EQ(matcher.get_matcher_id(), "dictionary");

    // Spanish Verbs
    EXPECT_TRUE(matcher.is_synonym("establecer", "fijar", "es"));
    EXPECT_TRUE(matcher.is_synonym("fijar", "disponer", "es"));
    EXPECT_TRUE(matcher.is_synonym("modificar", "alterar", "es"));
    EXPECT_TRUE(matcher.is_synonym("vulnerar", "infringir", "es"));
    EXPECT_TRUE(matcher.is_synonym("notificar", "comunicar", "es"));
    EXPECT_TRUE(matcher.is_synonym("requisito", "presupuesto", "es"));
    EXPECT_TRUE(matcher.is_synonym("tributo", "impuesto", "es"));
    EXPECT_TRUE(matcher.is_synonym("obligatorio", "preceptivo", "es"));

    // Non-synonyms
    EXPECT_FALSE(matcher.is_synonym("establecer", "derogar", "es"));
    EXPECT_FALSE(matcher.is_synonym("tributo", "sancion", "es"));
    EXPECT_FALSE(matcher.is_synonym("", "", "es"));
}

TEST(TestParaphraseMatcher, DictionaryEnglishSynonyms) {
    DictionaryParaphraseMatcher matcher;

    EXPECT_TRUE(matcher.is_synonym("establish", "determine", "en"));
    EXPECT_TRUE(matcher.is_synonym("modify", "amend", "en"));
    EXPECT_TRUE(matcher.is_synonym("terminate", "end", "en"));
    EXPECT_TRUE(matcher.is_synonym("prohibit", "forbid", "en"));
    EXPECT_TRUE(matcher.is_synonym("mandatory", "compulsory", "en"));

    EXPECT_FALSE(matcher.is_synonym("establish", "destroy", "en"));
}

TEST(TestParaphraseMatcher, DictionaryGetSynonymsList) {
    DictionaryParaphraseMatcher matcher;
    auto syns = matcher.get_synonyms("establecer", "es");
    EXPECT_FALSE(syns.empty());
    EXPECT_NE(std::find(syns.begin(), syns.end(), "fijar"), syns.end());
    EXPECT_NE(std::find(syns.begin(), syns.end(), "disponer"), syns.end());

    auto empty_syns = matcher.get_synonyms("palabra_inexistente_xyz", "es");
    EXPECT_TRUE(empty_syns.empty());
}

TEST(TestParaphraseMatcher, DictionaryCustomSynonymGroupAndDomainRule) {
    DictionaryParaphraseMatcher matcher;
    matcher.add_synonym_group({"hipoteca", "gravamen_inmobiliario", "derecho_real_de_garantia"}, "es");

    EXPECT_TRUE(matcher.is_synonym("hipoteca", "gravamen_inmobiliario", "es"));
    EXPECT_TRUE(matcher.is_synonym("hipoteca", "derecho_real_de_garantia", "es"));

    DomainParaphraseRule rule{"recurso potestativo", "impugnacion opcional", "law", 0.95f};
    matcher.add_domain_rule(rule);

    auto res = matcher.compare_phrases("recurso potestativo", "impugnacion opcional", "law", "es");
    EXPECT_TRUE(res.is_match);
    EXPECT_FLOAT_EQ(res.similarity_score, 0.95f);
}

TEST(TestParaphraseMatcher, DictionaryDomainParaphrases) {
    DictionaryParaphraseMatcher matcher;

    // Law domain phrases
    auto law1 = matcher.compare_phrases("hecho imponible", "presupuesto de hecho", "law", "es");
    EXPECT_TRUE(law1.is_match);
    EXPECT_TRUE(law1.is_multi_word_phrase);
    EXPECT_EQ(law1.ref_word_count, 2u);
    EXPECT_EQ(law1.trans_word_count, 3u);

    auto law2 = matcher.compare_phrases("entrar en vigor", "comenzar a regir", "law", "es");
    EXPECT_TRUE(law2.is_match);

    auto law3 = matcher.compare_phrases("sujeto pasivo", "obligado tributario", "law", "es");
    EXPECT_TRUE(law3.is_match);

    // Economics domain phrases
    auto econ1 = matcher.compare_phrases("tipo de interes", "precio del dinero", "economics", "es");
    EXPECT_TRUE(econ1.is_match);

    // Science domain phrases
    auto sci1 = matcher.compare_phrases("material genetico", "acido desoxirribonucleico", "science", "es");
    EXPECT_TRUE(sci1.is_match);

    // History domain phrases
    auto hist1 = matcher.compare_phrases("guerra civil", "conflicto belico espanol", "history", "es");
    EXPECT_TRUE(hist1.is_match);
}

TEST(TestParaphraseMatcher, DictionaryFindParaphrasesSequenceScan) {
    DictionaryParaphraseMatcher matcher;

    std::vector<std::string> ref_words = {"el", "hecho", "imponible", "establece", "la", "tasa"};
    std::vector<std::string> trans_words = {"el", "presupuesto", "de", "hecho", "fija", "la", "tasa"};

    auto matches = matcher.find_paraphrases(ref_words, trans_words, "law", "es");
    ASSERT_FALSE(matches.empty());

    // Should find multi-word paraphrase "hecho imponible" -> "presupuesto de hecho"
    bool found_multi = false;
    bool found_single = false;

    for (const auto& m : matches) {
        if (m.is_multi_word_phrase) {
            found_multi = true;
            EXPECT_EQ(m.ref_start_index, 1u);
            EXPECT_EQ(m.ref_word_count, 2u);
        } else if (m.matched_reference_phrase == "establece" || m.matched_reference_phrase == "establecer") {
            found_single = true;
        }
    }

    EXPECT_TRUE(found_multi);
    EXPECT_TRUE(found_single);
}

TEST(TestParaphraseMatcher, EmbeddingCosineSimilarityAndSafetyGuards) {
    // Exact identical vectors
    std::vector<float> v1 = {1.0f, 0.0f, 0.0f};
    std::vector<float> v2 = {1.0f, 0.0f, 0.0f};
    EXPECT_FLOAT_EQ(EmbeddingParaphraseMatcher::compute_cosine_similarity(v1, v2), 1.0f);

    // Orthogonal vectors
    std::vector<float> v3 = {0.0f, 1.0f, 0.0f};
    EXPECT_FLOAT_EQ(EmbeddingParaphraseMatcher::compute_cosine_similarity(v1, v3), 0.0f);

    // Empty vector handling
    EXPECT_FLOAT_EQ(EmbeddingParaphraseMatcher::compute_cosine_similarity({}, {}), 0.0f);

    // Safety guards: negation mismatch
    EXPECT_FALSE(EmbeddingParaphraseMatcher::passes_safety_guards("está permitido", "no está permitido"));
    EXPECT_FALSE(EmbeddingParaphraseMatcher::passes_safety_guards("nunca se aplica", "se aplica"));
    EXPECT_TRUE(EmbeddingParaphraseMatcher::passes_safety_guards("está permitido", "es facultativo"));

    // Safety guards: numeric mismatch
    EXPECT_FALSE(EmbeddingParaphraseMatcher::passes_safety_guards("articulo 15", "articulo 20"));
    EXPECT_FALSE(EmbeddingParaphraseMatcher::passes_safety_guards("plazo de 5 dias", "plazo de 10 dias"));
    EXPECT_TRUE(EmbeddingParaphraseMatcher::passes_safety_guards("articulo 15", "el precepto 15"));
}

TEST(TestParaphraseMatcher, EmbeddingParaphraseMatcherWithMock) {
    auto mock_engine = std::make_shared<testing::NiceMock<MockEmbeddingEngine>>();
    EmbeddingParaphraseMatcher matcher(mock_engine, 0.80f);
    EXPECT_EQ(matcher.get_matcher_id(), "embedding");

    std::vector<float> vec_ref = {1.0f, 0.5f, 0.0f};
    std::vector<float> vec_trans_close = {0.95f, 0.55f, 0.0f};
    std::vector<float> vec_trans_far = {0.0f, 0.0f, 1.0f};

    EXPECT_CALL(*mock_engine, generate_embedding("constituye el fundamento", _))
        .WillRepeatedly(Return(vec_ref));
    EXPECT_CALL(*mock_engine, generate_embedding("es la base principal", _))
        .WillRepeatedly(Return(vec_trans_close));
    EXPECT_CALL(*mock_engine, generate_embedding("concepto totalmente ajeno", _))
        .WillRepeatedly(Return(vec_trans_far));

    auto res_match = matcher.compare_phrases("constituye el fundamento", "es la base principal");
    EXPECT_TRUE(res_match.is_match);
    EXPECT_GT(res_match.similarity_score, 0.80f);

    auto res_far = matcher.compare_phrases("constituye el fundamento", "concepto totalmente ajeno");
    EXPECT_FALSE(res_far.is_match);

    std::vector<std::string> r_words = {"constituye", "el", "fundamento"};
    std::vector<std::string> t_words = {"es", "la", "base", "principal"};
    auto scan = matcher.find_paraphrases(r_words, t_words);
    EXPECT_FALSE(scan.empty());
}

TEST(TestParaphraseMatcher, HybridParaphraseMatcherTwoTierOperation) {
    auto mock_engine = std::make_shared<testing::NiceMock<MockEmbeddingEngine>>();
    ON_CALL(*mock_engine, generate_embedding(_, _))
        .WillByDefault(Return(std::vector<float>{}));
    EXPECT_CALL(*mock_engine, generate_embedding(_, _))
        .WillRepeatedly(Return(std::vector<float>{}));

    HybridParaphraseMatcher hybrid(mock_engine, 0.85f);
    EXPECT_EQ(hybrid.get_matcher_id(), "hybrid");

    // Tier 1 Fast Dictionary Hit (does NOT invoke mock engine)
    EXPECT_TRUE(hybrid.is_synonym("establecer", "fijar", "es"));
    auto dict_res = hybrid.compare_phrases("sujeto pasivo", "obligado tributario", "law", "es");
    EXPECT_TRUE(dict_res.is_match);

    // Tier 2 Fallback for unseen creative phrase
    std::vector<float> v_ref = {1.0f, 0.8f, 0.1f};
    std::vector<float> v_trans = {0.98f, 0.82f, 0.09f};

    EXPECT_CALL(*mock_engine, generate_embedding("redactar un informe", _))
        .WillRepeatedly(Return(v_ref));
    EXPECT_CALL(*mock_engine, generate_embedding("elaborar una memoria", _))
        .WillRepeatedly(Return(v_trans));

    auto embed_res = hybrid.compare_phrases("redactar un informe", "elaborar una memoria");
    EXPECT_TRUE(embed_res.is_match);
    EXPECT_GT(embed_res.similarity_score, 0.85f);

    // Test setters and getters on hybrid matcher
    hybrid.get_embedding_matcher().set_similarity_threshold(0.90f);
    EXPECT_FLOAT_EQ(hybrid.get_embedding_matcher().get_similarity_threshold(), 0.90f);
    hybrid.set_embedding_engine(mock_engine);

    // Test find_paraphrases on hybrid matcher with combined dictionary + embedding tokens
    std::vector<std::string> r_tokens = {"establece", "el", "informe"};
    std::vector<std::string> t_tokens = {"fija", "la", "memoria"};

    std::vector<float> v_inf = {1.0f, 0.0f};
    std::vector<float> v_mem = {0.98f, 0.02f};
    EXPECT_CALL(*mock_engine, generate_embedding("informe", _))
        .WillRepeatedly(Return(v_inf));
    EXPECT_CALL(*mock_engine, generate_embedding("memoria", _))
        .WillRepeatedly(Return(v_mem));

    auto hybrid_matches = hybrid.find_paraphrases(r_tokens, t_tokens);
    EXPECT_GE(hybrid_matches.size(), 1u);

    // Test find_paraphrases on hybrid matcher with empty input
    EXPECT_TRUE(hybrid.find_paraphrases({}, {}).empty());
}

TEST(TestParaphraseMatcher, EmbeddingMatcherSettersAndEdgeCases) {
    EmbeddingParaphraseMatcher matcher;
    EXPECT_FLOAT_EQ(matcher.get_similarity_threshold(), 0.85f);

    matcher.set_similarity_threshold(0.75f);
    EXPECT_FLOAT_EQ(matcher.get_similarity_threshold(), 0.75f);

    auto mock_engine = std::make_shared<testing::NiceMock<MockEmbeddingEngine>>();
    matcher.set_embedding_engine(mock_engine);

    // Empty phrase compare
    auto res_empty = matcher.compare_phrases("", "");
    EXPECT_FALSE(res_empty.is_match);

    // Empty find_paraphrases
    EXPECT_TRUE(matcher.find_paraphrases({}, {}).empty());

    // Reverse synonyms (always empty for embedding)
    EXPECT_TRUE(matcher.get_synonyms("palabra").empty());

    // is_synonym on embedding matcher
    std::vector<float> v1 = {1.0f, 0.0f};
    std::vector<float> v2 = {0.99f, 0.01f};
    EXPECT_CALL(*mock_engine, generate_embedding("termino", _))
        .WillRepeatedly(Return(v1));
    EXPECT_CALL(*mock_engine, generate_embedding("vocablo", _))
        .WillRepeatedly(Return(v2));
    EXPECT_TRUE(matcher.is_synonym("termino", "vocablo"));
}

TEST(TestParaphraseMatcher, ParaphraseMatcherManagerSingleton) {
    auto& manager = ParaphraseMatcherManager::getInstance();

    auto dict = manager.get_matcher("dictionary");
    ASSERT_NE(dict, nullptr);
    EXPECT_EQ(dict->get_matcher_id(), "dictionary");

    auto embed = manager.get_matcher("embedding");
    ASSERT_NE(embed, nullptr);
    EXPECT_EQ(embed->get_matcher_id(), "embedding");

    auto hybrid = manager.get_matcher("hybrid");
    ASSERT_NE(hybrid, nullptr);
    EXPECT_EQ(hybrid->get_matcher_id(), "hybrid");

    // Unknown falls back to hybrid
    auto unknown = manager.get_matcher("non_existent_matcher");
    ASSERT_NE(unknown, nullptr);
    EXPECT_EQ(unknown->get_matcher_id(), "hybrid");

    // Test set_embedding_engine propagation
    auto mock_engine = std::make_shared<MockEmbeddingEngine>();
    manager.set_embedding_engine(mock_engine);

    // Active matcher from configuration
    ConfigurationSystem::getInstance().set_value("SEMANTIC_PARAPHRASE", "default_matcher", "dictionary");
    auto active = manager.get_active_matcher();
    ASSERT_NE(active, nullptr);
    EXPECT_EQ(active->get_matcher_id(), "dictionary");

    ConfigurationSystem::getInstance().set_value("SEMANTIC_PARAPHRASE", "default_matcher", "hybrid");
    auto active_hybrid = manager.get_active_matcher();
    EXPECT_EQ(active_hybrid->get_matcher_id(), "hybrid");

    // Fallback when default_matcher config value is empty
    ConfigurationSystem::getInstance().set_value("SEMANTIC_PARAPHRASE", "default_matcher", "");
    auto active_empty = manager.get_active_matcher();
    EXPECT_EQ(active_empty->get_matcher_id(), "hybrid");
}
