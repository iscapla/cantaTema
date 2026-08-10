#include <gtest/gtest.h>
#include "similarity/faiss_similarity_search.hpp"
#include <vector>
#include <cmath>

TEST(FaissSimilaritySearchTest, IndexingAndCount) {
    FaissSimilaritySearch search;
    EXPECT_EQ(search.get_indexed_count(), 0);

    std::vector<std::vector<float>> trans = {
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f}
    };

    EXPECT_TRUE(search.index_transcript_embeddings(trans));
    EXPECT_EQ(search.get_indexed_count(), 2);

    search.reset();
    EXPECT_EQ(search.get_indexed_count(), 0);
}

TEST(FaissSimilaritySearchTest, EmptyIndexingAndErrors) {
    FaissSimilaritySearch search;
    
    // Empty index is accepted and clears index
    EXPECT_TRUE(search.index_transcript_embeddings({}));
    EXPECT_EQ(search.get_indexed_count(), 0);

    // First vector has size 0
    std::vector<std::vector<float>> invalid_empty = {
        {}
    };
    EXPECT_FALSE(search.index_transcript_embeddings(invalid_empty));

    // Dimension mismatch during index
    std::vector<std::vector<float>> mismatch = {
        {1.0f, 0.0f},
        {1.0f, 0.0f, 0.0f}
    };
    EXPECT_FALSE(search.index_transcript_embeddings(mismatch));
}

TEST(FaissSimilaritySearchTest, SearchWithNoIndex) {
    FaissSimilaritySearch search;
    
    std::vector<std::vector<float>> pdfs = {
        {1.0f, 0.0f, 0.0f}
    };
    std::vector<float> weights = {1.5f};
    
    // Search when nothing is indexed should return unmentioned with full weight missed
    auto results = search.search_pdf_matches(pdfs, weights, 0.75f);
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].best_transcript_chunk_index, -1);
    EXPECT_FLOAT_EQ(results[0].similarity_score, 0.0f);
    EXPECT_FALSE(results[0].is_mentioned);
    EXPECT_FLOAT_EQ(results[0].weighted_missed_score, 1.5f);
}

TEST(FaissSimilaritySearchTest, SearchWithEmptyInputs) {
    FaissSimilaritySearch search;
    std::vector<std::vector<float>> trans = {{1.0f, 0.0f}};
    EXPECT_TRUE(search.index_transcript_embeddings(trans));

    auto results = search.search_pdf_matches({}, {}, 0.75f);
    EXPECT_TRUE(results.empty());
}

TEST(FaissSimilaritySearchTest, ExactMatchAndSimilarityScoring) {
    FaissSimilaritySearch search;
    std::vector<std::vector<float>> trans = {
        {1.0f, 0.0f, 0.0f}, // chunk 0
        {0.0f, 1.0f, 0.0f}  // chunk 1
    };
    EXPECT_TRUE(search.index_transcript_embeddings(trans));

    std::vector<std::vector<float>> pdfs = {
        {1.0f, 0.0f, 0.0f}, // matches chunk 0 exactly
        {0.0f, 1.0f, 0.0f}, // matches chunk 1 exactly
        {0.0f, 0.0f, 1.0f}  // orthogonal to both (similarity 0.0)
    };
    std::vector<float> weights = {1.2f, 1.5f, 2.0f};

    auto results = search.search_pdf_matches(pdfs, weights, 0.75f);
    ASSERT_EQ(results.size(), 3);

    // PDF 0 matches transcript 0
    EXPECT_EQ(results[0].pdf_chunk_index, 0);
    EXPECT_EQ(results[0].best_transcript_chunk_index, 0);
    EXPECT_NEAR(results[0].similarity_score, 1.0f, 1e-5f);
    EXPECT_TRUE(results[0].is_mentioned);
    EXPECT_FLOAT_EQ(results[0].weighted_missed_score, 0.0f);

    // PDF 1 matches transcript 1
    EXPECT_EQ(results[1].pdf_chunk_index, 1);
    EXPECT_EQ(results[1].best_transcript_chunk_index, 1);
    EXPECT_NEAR(results[1].similarity_score, 1.0f, 1e-5f);
    EXPECT_TRUE(results[1].is_mentioned);
    EXPECT_FLOAT_EQ(results[1].weighted_missed_score, 0.0f);

    // PDF 2 doesn't match either (similarity score 0.0 < threshold 0.75), best_transcript_chunk_index should be -1
    EXPECT_EQ(results[2].pdf_chunk_index, 2);
    EXPECT_EQ(results[2].best_transcript_chunk_index, -1);
    EXPECT_GE(results[2].candidate_transcript_chunk_index, 0);
    EXPECT_NEAR(results[2].similarity_score, 0.0f, 1e-5f);
    EXPECT_FALSE(results[2].is_mentioned);
    // missed score = weight * (1.0 - similarity) = 2.0 * (1.0 - 0.0) = 2.0
    EXPECT_FLOAT_EQ(results[2].weighted_missed_score, 2.0f);
}

TEST(FaissSimilaritySearchTest, ThresholdAndPartialMatch) {
    FaissSimilaritySearch search;
    // Normalized vectors:
    // A = [1.0, 0.0]
    std::vector<std::vector<float>> trans = {
        {1.0f, 0.0f}
    };
    EXPECT_TRUE(search.index_transcript_embeddings(trans));

    // B = [0.7071f, 0.7071f] (45 degrees, cosine similarity ~ 0.7071)
    std::vector<std::vector<float>> pdfs = {
        {0.7071f, 0.7071f}
    };
    std::vector<float> weights = {1.5f};

    // Case 1: Threshold = 0.70 (similarity 0.7071 >= 0.70 => mentioned)
    {
        auto results = search.search_pdf_matches(pdfs, weights, 0.70f);
        ASSERT_EQ(results.size(), 1);
        EXPECT_TRUE(results[0].is_mentioned);
        EXPECT_FLOAT_EQ(results[0].weighted_missed_score, 0.0f);
    }

    // Case 2: Threshold = 0.75 (similarity 0.7071 < 0.75 => not mentioned)
    {
        auto results = search.search_pdf_matches(pdfs, weights, 0.75f);
        ASSERT_EQ(results.size(), 1);
        EXPECT_FALSE(results[0].is_mentioned);
        float expected_missed = 1.5f * (1.0f - results[0].similarity_score);
        EXPECT_NEAR(results[0].weighted_missed_score, expected_missed, 1e-5f);
    }
}

TEST(FaissSimilaritySearchTest, DimensionMismatchAndZeroNorm) {
    FaissSimilaritySearch search;
    std::vector<std::vector<float>> trans = {
        {1.0f, 0.0f, 0.0f}
    };
    EXPECT_TRUE(search.index_transcript_embeddings(trans));

    // PDF with dimension mismatch (size 2 instead of 3)
    std::vector<std::vector<float>> pdfs = {
        {1.0f, 0.0f}
    };
    std::vector<float> weights = {1.5f};

    auto results = search.search_pdf_matches(pdfs, weights, 0.75f);
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].best_transcript_chunk_index, -1);
    EXPECT_FALSE(results[0].is_mentioned);
    EXPECT_FLOAT_EQ(results[0].weighted_missed_score, 1.5f);

    // PDF with zero-norm vector
    std::vector<std::vector<float>> zero_pdfs = {
        {0.0f, 0.0f, 0.0f}
    };
    results = search.search_pdf_matches(zero_pdfs, weights, 0.75f);
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].best_transcript_chunk_index, -1);
    EXPECT_FALSE(results[0].is_mentioned);
    EXPECT_FLOAT_EQ(results[0].weighted_missed_score, 1.5f);
}

TEST(FaissSimilaritySearchTest, NonNormalizedInputs) {
    FaissSimilaritySearch search;
    // Index non-normalized vector [2.0, 0.0, 0.0]
    std::vector<std::vector<float>> trans = {
        {2.0f, 0.0f, 0.0f}
    };
    EXPECT_TRUE(search.index_transcript_embeddings(trans));

    // Query with non-normalized vector [0.0, 3.0, 0.0]
    std::vector<std::vector<float>> pdfs = {
        {0.0f, 3.0f, 0.0f},
        {1.5f, 0.0f, 0.0f}
    };
    std::vector<float> weights = {1.0f, 1.0f};

    auto results = search.search_pdf_matches(pdfs, weights, 0.75f);
    ASSERT_EQ(results.size(), 2);
    
    // First should be orthogonal (similarity 0)
    EXPECT_NEAR(results[0].similarity_score, 0.0f, 1e-5f);
    EXPECT_FALSE(results[0].is_mentioned);
    
    // Second should match exactly (similarity 1.0)
    EXPECT_NEAR(results[1].similarity_score, 1.0f, 1e-5f);
    EXPECT_TRUE(results[1].is_mentioned);
}

TEST(FaissSimilaritySearchTest, AdvancedSearchCandidateTrackingAndNumericWarnings) {
    FaissSimilaritySearch search;
    std::vector<std::vector<float>> trans = {
        {1.0f, 0.0f, 0.0f} // index 0
    };
    EXPECT_TRUE(search.index_transcript_embeddings(trans));

    std::vector<std::vector<float>> pdfs = {
        {0.5f, 0.5f, 0.0f} // similarity ~0.707
    };
    std::vector<std::string> pdf_texts = {"Ocurrió en 1978 según el artículo 14."};
    std::vector<std::string> transcript_texts = {"Menciona el año 1995."};
    std::vector<float> weights = {1.0f};

    SimilaritySearchOptions options;
    options.similarity_threshold = 0.90f; // High threshold -> best_transcript_chunk_index should be -1
    options.numeric_boost = 0.10f;
    options.numeric_mismatch_penalty = 0.15f;

    auto results = search.search_pdf_matches_advanced(pdfs, pdf_texts, transcript_texts, weights, options);
    ASSERT_EQ(results.size(), 1);

    // Should NOT be mentioned because 0.707 - 0.15 = 0.557 < 0.90
    EXPECT_FALSE(results[0].is_mentioned);
    EXPECT_EQ(results[0].best_transcript_chunk_index, -1);
    EXPECT_EQ(results[0].candidate_transcript_chunk_index, 0);
    EXPECT_TRUE(results[0].has_numeric_warning);
}

TEST(FaissSimilaritySearchTest, EnumerationSequenceOrderTolerance) {
    FaissSimilaritySearch search;
    std::vector<std::vector<float>> trans = {
        {0.0f, 1.0f, 0.0f}, // segment 0 (matches item B)
        {1.0f, 0.0f, 0.0f}  // segment 1 (matches item A)
    };
    EXPECT_TRUE(search.index_transcript_embeddings(trans));

    std::vector<std::vector<float>> pdfs = {
        {1.0f, 0.0f, 0.0f}, // pdf 0: item A
        {0.0f, 1.0f, 0.0f}  // pdf 1: item B
    };
    std::vector<std::string> pdf_texts = {
        "1. Primer punto fundamental",
        "2. Segundo punto clave"
    };
    std::vector<std::string> transcript_texts = {
        "Segundo punto clave expuesto.",
        "Primer punto fundamental detallado."
    };
    std::vector<float> weights = {1.0f, 1.0f};

    SimilaritySearchOptions options;
    options.similarity_threshold = 0.50f;
    options.temporal_penalty_weight = 0.20f; // Strong penalty if order enforced

    auto results = search.search_pdf_matches_advanced(pdfs, pdf_texts, transcript_texts, weights, options);
    ASSERT_EQ(results.size(), 2);

    // Item A (PDF 0) should match transcript segment 1
    EXPECT_TRUE(results[0].is_mentioned);
    EXPECT_EQ(results[0].best_transcript_chunk_index, 1);

    // Item B (PDF 1) should match transcript segment 0 (out of order, but NO penalty because it's enumerated!)
    EXPECT_TRUE(results[1].is_mentioned);
    EXPECT_EQ(results[1].best_transcript_chunk_index, 0);
}

TEST(FaissSimilaritySearchTest, ShortHeadingFalsePositiveResolution) {
    FaissSimilaritySearch search;
    // Single transcript embedding with high cosine vector similarity (~0.834)
    std::vector<std::vector<float>> trans = {
        {0.834f, 0.551f, 0.0f}
    };
    EXPECT_TRUE(search.index_transcript_embeddings(trans));

    // PDF 0: Short heading "Naturaleza del Impuesto." (3 words <= 10)
    std::vector<std::vector<float>> pdfs = {
        {1.0f, 0.0f, 0.0f}
    };
    std::vector<std::string> pdf_texts = {"Naturaleza del Impuesto."};
    std::vector<std::string> transcript_texts = {"cuando el contribuyente hubiera renunciado a su aplicación."};
    std::vector<float> weights = {1.0f};

    SimilaritySearchOptions options;
    options.similarity_threshold = 0.65f;
    options.short_chunk_word_threshold = 10u;
    options.lexical_mismatch_scaling_factor = 0.60f;

    auto results = search.search_pdf_matches_advanced(pdfs, pdf_texts, transcript_texts, weights, options);
    ASSERT_EQ(results.size(), 1);

    // Raw similarity 0.834 scaled by 0.60 = 0.5004 < 0.65 threshold -> should NOT be mentioned!
    EXPECT_FALSE(results[0].is_mentioned);
    EXPECT_EQ(results[0].best_transcript_chunk_index, -1);
    EXPECT_EQ(results[0].candidate_transcript_chunk_index, 0);
    EXPECT_NEAR(results[0].similarity_score, 0.5004f, 1e-3f);
}


