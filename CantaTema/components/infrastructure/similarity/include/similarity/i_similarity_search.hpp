/**
 * @file i_similarity_search.hpp
 * @brief Defines the abstract interface for the similarity search subsystem.
 */

#ifndef I_SIMILARITY_SEARCH_HPP
#define I_SIMILARITY_SEARCH_HPP

#include <vector>
#include <string>
#include <cstddef>

#include "similarity/word_sequence_aligner.hpp"

/**
 * @brief Struct representing the match result for a single PDF chunk.
 */
struct SimilarityResult {
    /// Index of the PDF reference chunk.
    size_t pdf_chunk_index = 0;
    
    /// Index of the best matching transcript chunk in the index. -1 if no match or below threshold.
    int best_transcript_chunk_index = -1;
    
    /// Index of top candidate transcript chunk, even if similarity is below threshold. -1 if no candidates.
    int candidate_transcript_chunk_index = -1;

    /// The cosine/inner product similarity score between the two chunks.
    float similarity_score = 0.0f;
    
    /// True if similarity_score >= similarity_threshold.
    bool is_mentioned = false;
    
    /// True if numerical entities (dates, numbers) were misquoted or contradicted.
    bool has_numeric_warning = false;

    /// Weighted score calculated as: importance_weight * (1.0 - similarity_score) if not mentioned, or 0.0 if mentioned.
    float weighted_missed_score = 0.0f;

    /// Granular word-level alignment result (matched vs omitted words)
    WordAlignmentResult word_alignment;

    /// Percentage of reference words spoken correctly in the matched transcript segment (0.0 to 1.0)
    float word_recall_score = 0.0f;
};

/**
 * @brief Advanced options for order alignment and numerical entity scoring.
 */
struct SimilaritySearchOptions {
    float similarity_threshold = 0.65f;
    float numeric_boost = 0.10f;
    float numeric_mismatch_penalty = 0.15f;
    float temporal_penalty_weight = 0.05f;
    unsigned int short_chunk_word_threshold = 10u;
    float lexical_mismatch_scaling_factor = 0.60f;
};

/**
 * @brief Abstract interface for similarity search operations.
 */
class ISimilaritySearch {
public:
    virtual ~ISimilaritySearch() = default;

    /**
     * @brief Indexes a list of transcript embedding vectors.
     * 
     * @param transcript_embeddings The list of embedding vectors to index.
     * @return true if indexing succeeded, false otherwise.
     */
    virtual bool index_transcript_embeddings(const std::vector<std::vector<float>>& transcript_embeddings) = 0;

    /**
     * @brief Queries the indexed transcript embeddings for each PDF chunk embedding to find the best match.
     * 
     * @param pdf_embeddings The list of PDF chunk embedding vectors.
     * @param importance_weights The importance weight for each PDF chunk.
     * @param similarity_threshold The cosine similarity threshold to classify a chunk as "mentioned".
     * @return std::vector<SimilarityResult> List of similarity results corresponding to each PDF chunk.
     */
    virtual std::vector<SimilarityResult> search_pdf_matches(
        const std::vector<std::vector<float>>& pdf_embeddings,
        const std::vector<float>& importance_weights,
        float similarity_threshold) = 0;

    /**
     * @brief Queries indexed transcript embeddings with order-awareness and numerical entity validation.
     */
    virtual std::vector<SimilarityResult> search_pdf_matches_advanced(
        const std::vector<std::vector<float>>& pdf_embeddings,
        const std::vector<std::string>& pdf_texts,
        const std::vector<std::string>& transcript_texts,
        const std::vector<float>& importance_weights,
        const SimilaritySearchOptions& options) {
        return search_pdf_matches(pdf_embeddings, importance_weights, options.similarity_threshold);
    }

    /**
     * @brief Clears the current indexed transcript embeddings.
     */
    virtual void reset() = 0;

    /**
     * @brief Gets the number of currently indexed transcript embeddings.
     * 
     * @return size_t Number of indexed vectors.
     */
    virtual size_t get_indexed_count() const = 0;
};

#endif // I_SIMILARITY_SEARCH_HPP
