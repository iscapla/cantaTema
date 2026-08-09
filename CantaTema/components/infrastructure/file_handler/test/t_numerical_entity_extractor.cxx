/**
 * @file t_numerical_entity_extractor.cxx
 * @brief Unit tests for NumericalEntityExtractor class.
 */

#include <gtest/gtest.h>
#include "file_handler/numerical_entity_extractor.hpp"

TEST(NumericalEntityExtractorTest, DetectsEnumeratedItems) {
    EXPECT_TRUE(NumericalEntityExtractor::is_enumerated_item("1. Primera sección de la ley"));
    EXPECT_TRUE(NumericalEntityExtractor::is_enumerated_item("a) Principios generales"));
    EXPECT_TRUE(NumericalEntityExtractor::is_enumerated_item("• Punto clave"));
    EXPECT_TRUE(NumericalEntityExtractor::is_enumerated_item("- Otro punto"));
    EXPECT_TRUE(NumericalEntityExtractor::is_enumerated_item("1º Primer derecho"));
    EXPECT_TRUE(NumericalEntityExtractor::is_enumerated_item("Artículo 14 de la Constitución"));
    EXPECT_TRUE(NumericalEntityExtractor::is_enumerated_item("Art. 5 de la norma"));
    EXPECT_TRUE(NumericalEntityExtractor::is_enumerated_item("Primero: Disposición general"));

    EXPECT_FALSE(NumericalEntityExtractor::is_enumerated_item("La Constitución Española de 1978 establece los derechos fundamentales."));
    EXPECT_FALSE(NumericalEntityExtractor::is_enumerated_item("En este capítulo analizamos la economía mundial."));
}

TEST(NumericalEntityExtractorTest, ExtractsDatesAndNumbers) {
    std::string text = "Aprobada el 06/12/1978 con el 87.8% de los votos según el Artículo 14.";
    auto entities = NumericalEntityExtractor::extract_entities(text);

    EXPECT_TRUE(entities.count("1978") > 0 || entities.count("06/12/1978") > 0);
    EXPECT_TRUE(entities.count("87.8%") > 0 || entities.count("87.8") > 0 || entities.count("87") > 0);
    EXPECT_TRUE(entities.count("art14") > 0 || entities.count("14") > 0);
}

TEST(NumericalEntityExtractorTest, ComparesEntitiesExactMatch) {
    std::string ref = "La ley fue aprobada en 1978 por el Artículo 14.";
    std::string trans = "Se aprobó en 1978 bajo el artículo 14.";

    auto analysis = NumericalEntityExtractor::compare_entities(ref, trans, 0.10f, 0.15f);
    EXPECT_EQ(analysis.status, NumericResult::EXACT_MATCH);
    EXPECT_FALSE(analysis.has_warning);
    EXPECT_GT(analysis.score_modifier, 0.0f);
}

TEST(NumericalEntityExtractorTest, ComparesEntitiesMismatchWarning) {
    std::string ref = "Ocurrió en el año 1978.";
    std::string trans = "Sucedió en el año 1995.";

    auto analysis = NumericalEntityExtractor::compare_entities(ref, trans, 0.10f, 0.15f);
    EXPECT_EQ(analysis.status, NumericResult::MISMATCH);
    EXPECT_TRUE(analysis.has_warning);
    EXPECT_LT(analysis.score_modifier, 0.0f);
}
