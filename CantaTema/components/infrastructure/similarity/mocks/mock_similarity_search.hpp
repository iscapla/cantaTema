/**
 * @file mock_similarity_search.hpp
 * @brief Defines the GMock class for ISimilaritySearch.
 */

#ifndef MOCK_SIMILARITY_SEARCH_HPP
#define MOCK_SIMILARITY_SEARCH_HPP

#include <gmock/gmock.h>
#include "similarity/i_similarity_search.hpp"

class MockSimilaritySearch : public ISimilaritySearch {
public:
    MOCK_METHOD(bool, index_transcript_embeddings, (const std::vector<std::vector<float>>& transcript_embeddings), (override));
    MOCK_METHOD(std::vector<SimilarityResult>, search_pdf_matches, 
                (const std::vector<std::vector<float>>& pdf_embeddings, 
                 const std::vector<float>& importance_weights, 
                 float similarity_threshold), (override));
    MOCK_METHOD(std::vector<SimilarityResult>, search_pdf_matches_advanced, (
        const std::vector<std::vector<float>>& pdf_embeddings,
        const std::vector<std::string>& pdf_texts,
        const std::vector<std::string>& transcript_texts,
        const std::vector<float>& importance_weights,
        const SimilaritySearchOptions& options), (override));
    MOCK_METHOD(void, reset, (), (override));
    MOCK_METHOD(size_t, get_indexed_count, (), (const, override));
};

#endif // MOCK_SIMILARITY_SEARCH_HPP
