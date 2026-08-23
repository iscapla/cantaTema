/**
 * @file t_rubric_checklist_extractor.cxx
 * @brief Unit tests for RubricChecklistExtractor and RubricScorecard verification engine.
 */

#include <gtest/gtest.h>
#include "similarity/rubric_checklist_extractor.hpp"

TEST(RubricChecklistExtractorTest, Normalization) {
    EXPECT_EQ(RubricChecklistExtractor::normalize_text("Artículo"), "articulo");
    EXPECT_EQ(RubricChecklistExtractor::normalize_text("¿Ley?"), "ley");
}

TEST(RubricChecklistExtractorTest, ExtractLegalArticlesAndLaws) {
    std::vector<std::string> chunks = {
        "El Artículo 14 de la Constitución garantiza la igualdad ante la ley.",
        "Conforme a la Ley 39/2015 del Procedimiento Administrativo Común.",
        "Según el RD 1/2020 por el que se aprueba el texto refundido."
    };

    auto items = RubricChecklistExtractor::extract_rubric_items(chunks, "law");
    ASSERT_FALSE(items.empty());

    bool found_art14 = false;
    bool found_ley = false;
    bool found_rd = false;

    for (const auto& item : items) {
        if (item.entity_type == RubricEntityType::LEGAL_ARTICLE && item.normalized_text.find("articulo 14") != std::string::npos) {
            found_art14 = true;
        }
        if (item.entity_type == RubricEntityType::LAW_ID && item.normalized_text.find("ley") != std::string::npos) {
            found_ley = true;
        }
        if (item.entity_type == RubricEntityType::LAW_ID && item.normalized_text.find("rd") != std::string::npos) {
            found_rd = true;
        }
    }

    EXPECT_TRUE(found_art14);
    EXPECT_TRUE(found_ley);
    EXPECT_TRUE(found_rd);
}

TEST(RubricChecklistExtractorTest, ExtractDatesCenturiesAndMetrics) {
    std::vector<std::string> chunks = {
        "La Constitución Española de 1978 en el Siglo XX estableció un 50% de representación.",
        "Se aplicará una tasa del 15% sobre el valor total."
    };

    auto items = RubricChecklistExtractor::extract_rubric_items(chunks, "history");
    ASSERT_FALSE(items.empty());

    bool found_year = false;
    bool found_century = false;
    bool found_pct = false;

    for (const auto& item : items) {
        if (item.entity_type == RubricEntityType::DATE_OR_ERA && item.normalized_text.find("1978") != std::string::npos) {
            found_year = true;
        }
        if (item.entity_type == RubricEntityType::DATE_OR_ERA && item.normalized_text.find("siglo xx") != std::string::npos) {
            found_century = true;
        }
        if (item.entity_type == RubricEntityType::NUMERIC_METRIC) {
            found_pct = true;
        }
    }

    EXPECT_TRUE(found_year);
    EXPECT_TRUE(found_century);
    EXPECT_TRUE(found_pct);
}

TEST(RubricChecklistExtractorTest, ExtractScientificAndEnumeratedPoints) {
    std::vector<std::string> chunks = {
        "La molécula de ADN y la síntesis de ATP en la mitocondria.",
        "Se distinguen los siguientes requisitos: a) ser mayor de edad, b) poseer la titulación."
    };

    auto items = RubricChecklistExtractor::extract_rubric_items(chunks, "science");
    ASSERT_FALSE(items.empty());

    bool found_adn = false;
    bool found_atp = false;
    bool found_enum = false;

    for (const auto& item : items) {
        if (item.entity_type == RubricEntityType::SCIENTIFIC_TERM && item.normalized_text == "adn") {
            found_adn = true;
        }
        if (item.entity_type == RubricEntityType::SCIENTIFIC_TERM && item.normalized_text == "atp") {
            found_atp = true;
        }
        if (item.entity_type == RubricEntityType::ENUMERATED_POINT) {
            found_enum = true;
        }
    }

    EXPECT_TRUE(found_adn);
    EXPECT_TRUE(found_atp);
    EXPECT_TRUE(found_enum);
}

TEST(RubricChecklistExtractorTest, EvaluateRubricChecklist) {
    std::vector<std::string> ref_chunks = {
        "El Artículo 14 reconoce la igualdad de todos los españoles.",
        "La Ley 39/2015 regula el procedimiento.",
        "Se aprobó en el año 1978."
    };

    auto rubric_items = RubricChecklistExtractor::extract_rubric_items(ref_chunks, "law");
    ASSERT_GE(rubric_items.size(), 3);

    // Case 1: Spoken transcript mentions article 14 and year 1978, but omits Ley 39/2015
    std::vector<std::string> transcript_segments = {
        "Hola en esta exposicion hablaremos del articulo 14 sobre igualdad",
        "todo esto se aprobo en 1978 tras la constitucion"
    };

    auto scorecard = RubricChecklistExtractor::evaluate_rubric(rubric_items, transcript_segments, true);

    EXPECT_EQ(scorecard.total_items, rubric_items.size());
    EXPECT_GE(scorecard.satisfied_items, 2);
    EXPECT_GE(scorecard.omitted_items, 1);
    EXPECT_GT(scorecard.citation_accuracy_pct, 0.0f);
    EXPECT_LT(scorecard.citation_accuracy_pct, 100.0f);

    // Check individual items
    for (const auto& item : scorecard.items) {
        if (item.normalized_text.find("articulo 14") != std::string::npos) {
            EXPECT_TRUE(item.is_satisfied);
            EXPECT_EQ(item.matched_transcript_index, 0);
        }
        if (item.normalized_text.find("1978") != std::string::npos) {
            EXPECT_TRUE(item.is_satisfied);
            EXPECT_EQ(item.matched_transcript_index, 1);
        }
    }
}

TEST(RubricChecklistExtractorTest, EmptyInputs) {
    std::vector<std::string> empty_chunks;
    auto items = RubricChecklistExtractor::extract_rubric_items(empty_chunks);
    EXPECT_TRUE(items.empty());

    auto scorecard = RubricChecklistExtractor::evaluate_rubric(items, {});
    EXPECT_EQ(scorecard.total_items, 0);
    EXPECT_FLOAT_EQ(scorecard.citation_accuracy_pct, 100.0f);
}
