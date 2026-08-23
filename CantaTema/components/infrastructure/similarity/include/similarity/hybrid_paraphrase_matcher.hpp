/**
 * @file hybrid_paraphrase_matcher.hpp
 * @brief Hybrid Two-Tier semantic paraphrase matcher (Tier 1 Lexicon + Tier 2 Neural Embedding).
 */

#ifndef HYBRID_PARAPHRASE_MATCHER_HPP
#define HYBRID_PARAPHRASE_MATCHER_HPP

#include "similarity/i_paraphrase_matcher.hpp"
#include "similarity/dictionary_paraphrase_matcher.hpp"
#include "similarity/embedding_paraphrase_matcher.hpp"
#include <memory>

/**
 * @class HybridParaphraseMatcher
 * @brief Combines Tier 1 deterministic curated dictionary lookups with Tier 2 dynamic neural embeddings.
 */
class HybridParaphraseMatcher : public IParaphraseMatcher {
public:
    explicit HybridParaphraseMatcher(
        std::shared_ptr<IEmbeddingEngine> embedding_engine = nullptr,
        float embedding_threshold = 0.85f
    );
    ~HybridParaphraseMatcher() override = default;

    std::string get_matcher_id() const override { return "hybrid"; }

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

    DictionaryParaphraseMatcher& get_dictionary_matcher() { return m_dict_matcher; }
    EmbeddingParaphraseMatcher& get_embedding_matcher() { return m_embed_matcher; }

    void set_embedding_engine(std::shared_ptr<IEmbeddingEngine> engine) {
        m_embed_matcher.set_embedding_engine(std::move(engine));
    }

private:
    DictionaryParaphraseMatcher m_dict_matcher;
    EmbeddingParaphraseMatcher m_embed_matcher;
};

#endif // HYBRID_PARAPHRASE_MATCHER_HPP
