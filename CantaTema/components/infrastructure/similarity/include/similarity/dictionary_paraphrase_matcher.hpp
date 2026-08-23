/**
 * @file dictionary_paraphrase_matcher.hpp
 * @brief Tier 1 deterministic synonym and domain paraphrase matcher using curated lexicons.
 */

#ifndef DICTIONARY_PARAPHRASE_MATCHER_HPP
#define DICTIONARY_PARAPHRASE_MATCHER_HPP

#include "similarity/i_paraphrase_matcher.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>

/**
 * @struct DomainParaphraseRule
 * @brief Multi-word domain paraphrase mapping rule.
 */
struct DomainParaphraseRule {
    std::string ref_phrase;
    std::string trans_phrase;
    std::string domain_key;
    float confidence = 1.0f;
};

/**
 * @class DictionaryParaphraseMatcher
 * @brief Implements Tier 1 deterministic lexicon and domain multi-word paraphrase matching for Spanish and English.
 */
class DictionaryParaphraseMatcher : public IParaphraseMatcher {
public:
    DictionaryParaphraseMatcher();
    ~DictionaryParaphraseMatcher() override = default;

    std::string get_matcher_id() const override { return "dictionary"; }

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

    /**
     * @brief Adds a custom synonym group at runtime.
     * @param words List of equivalent words.
     * @param language Target language.
     */
    void add_synonym_group(const std::vector<std::string>& words, const std::string& language = "es");

    /**
     * @brief Adds a custom domain phrase rule at runtime.
     * @param rule Domain paraphrase rule.
     */
    void add_domain_rule(const DomainParaphraseRule& rule);

private:
    void initialize_spanish_synonyms();
    void initialize_english_synonyms();
    void initialize_domain_rules();

    // Word -> Synset cluster ID
    std::unordered_map<std::string, size_t> m_es_word_to_cluster;
    std::vector<std::vector<std::string>> m_es_synset_clusters;

    std::unordered_map<std::string, size_t> m_en_word_to_cluster;
    std::vector<std::vector<std::string>> m_en_synset_clusters;

    // Multi-word phrase rules
    std::vector<DomainParaphraseRule> m_domain_rules;
};

#endif // DICTIONARY_PARAPHRASE_MATCHER_HPP
