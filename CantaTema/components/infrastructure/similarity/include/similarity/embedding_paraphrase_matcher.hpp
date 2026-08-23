/**
 * @file embedding_paraphrase_matcher.hpp
 * @brief Tier 2 neural embedding semantic paraphrase matcher using local IEmbeddingEngine.
 */

#ifndef EMBEDDING_PARAPHRASE_MATCHER_HPP
#define EMBEDDING_PARAPHRASE_MATCHER_HPP

#include "similarity/i_paraphrase_matcher.hpp"
#include "embeddings/i_embedding_engine.hpp"
#include <memory>
#include <vector>
#include <string>

/**
 * @class EmbeddingParaphraseMatcher
 * @brief Uses local llama.cpp / E5 vector embeddings and cosine similarity to match open-ended paraphrases with safety guards.
 */
class EmbeddingParaphraseMatcher : public IParaphraseMatcher {
public:
    explicit EmbeddingParaphraseMatcher(
        std::shared_ptr<IEmbeddingEngine> engine = nullptr,
        float similarity_threshold = 0.85f
    );
    ~EmbeddingParaphraseMatcher() override = default;

    std::string get_matcher_id() const override { return "embedding"; }

    bool is_synonym(const std::string& word1, const std::string& word2, const std::string& language = "es") const override;

    std::vector<std::string> get_synonyms(const std::string& word, const std::string& language = "es") const override;

    ParaphraseMatchResult compare_phrases(
        const std::string& ref_phrase,
        const std::string& trans_phrase,
        const std::string& domain_key = "general",
        const std::string& language = "es"
    ) const override;

    std::vector<ParaphraseMatchResult> find_paraphrases(
        const std::vector<std::string>& ref_words,
        const std::vector<std::string>& trans_words,
        const std::string& domain_key = "general",
        const std::string& language = "es"
    ) const override;

    void set_embedding_engine(std::shared_ptr<IEmbeddingEngine> engine) { m_engine = std::move(engine); }
    void set_similarity_threshold(float threshold) { m_threshold = threshold; }
    float get_similarity_threshold() const { return m_threshold; }

    /**
     * @brief Evaluates domain safety guards (e.g. verifying negation and numeric consistency).
     * @param ref_span Reference text.
     * @param trans_span Candidate transcript text.
     * @return bool True if safe to match semantically, false if safety violation.
     */
    static bool passes_safety_guards(const std::string& ref_span, const std::string& trans_span);

    /**
     * @brief Computes cosine similarity between two float vectors.
     */
    static float compute_cosine_similarity(const std::vector<float>& vec1, const std::vector<float>& vec2);

private:
    std::shared_ptr<IEmbeddingEngine> m_engine;
    float m_threshold = 0.85f;
};

#endif // EMBEDDING_PARAPHRASE_MATCHER_HPP
