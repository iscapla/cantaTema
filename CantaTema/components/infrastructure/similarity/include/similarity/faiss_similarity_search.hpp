/**
 * @file faiss_similarity_search.hpp
 * @brief Declares the FaissSimilaritySearch class implementing ISimilaritySearch.
 */

#ifndef FAISS_SIMILARITY_SEARCH_HPP
#define FAISS_SIMILARITY_SEARCH_HPP

#include "similarity/i_similarity_search.hpp"
#include <vector>
#include <mutex>

/**
 * @brief Implementation of ISimilaritySearch using an exact inner product/cosine similarity search.
 */
class FaissSimilaritySearch : public ISimilaritySearch {
public:
    FaissSimilaritySearch() = default;
    ~FaissSimilaritySearch() override = default;

    /**
     * @brief Indexes a list of transcript embedding vectors.
     * 
     * @param transcript_embeddings The list of embedding vectors to index.
     * @return true if indexing succeeded, false otherwise.
     */
    bool index_transcript_embeddings(const std::vector<std::vector<float>>& transcript_embeddings) override;

    /**
     * @brief Queries the indexed transcript embeddings for each PDF chunk embedding to find the best match.
     * 
     * @param pdf_embeddings The list of PDF chunk embedding vectors.
     * @param importance_weights The importance weight for each PDF chunk.
     * @param similarity_threshold The cosine similarity threshold to classify a chunk as "mentioned".
     * @return std::vector<SimilarityResult> List of similarity results corresponding to each PDF chunk.
     */
    std::vector<SimilarityResult> search_pdf_matches(
        const std::vector<std::vector<float>>& pdf_embeddings,
        const std::vector<float>& importance_weights,
        float similarity_threshold) override;

    /**
     * @brief Clears the current indexed transcript embeddings.
     */
    void reset() override;

    /**
     * @brief Gets the number of currently indexed transcript embeddings.
     * 
     * @return size_t Number of indexed vectors.
     */
    size_t get_indexed_count() const override;

private:
    mutable std::mutex m_mutex;
    std::vector<std::vector<float>> m_indexed_embeddings;
    size_t m_dimension = 0;
};

#endif // FAISS_SIMILARITY_SEARCH_HPP
