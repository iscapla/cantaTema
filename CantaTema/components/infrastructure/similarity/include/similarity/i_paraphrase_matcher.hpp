/**
 * @file i_paraphrase_matcher.hpp
 * @brief Abstract interface and models for synonym and domain paraphrase semantic matching.
 */

#ifndef I_PARAPHRASE_MATCHER_HPP
#define I_PARAPHRASE_MATCHER_HPP

#include <string>
#include <vector>
#include <cstddef>

/**
 * @struct ParaphraseMatchResult
 * @brief Result of comparing two words or phrases for semantic paraphrase equivalence.
 */
struct ParaphraseMatchResult {
    bool is_match = false;                    ///< True if expressions represent valid synonyms or domain paraphrases
    float similarity_score = 0.0f;            ///< Semantic equivalence confidence score [0.0 - 1.0]
    std::string matched_reference_phrase;     ///< Reference word or phrase matched
    std::string matched_transcript_phrase;    ///< Spoken transcript word or phrase matched
    size_t ref_start_index = 0;               ///< Starting index in reference word list
    size_t ref_word_count = 1;                ///< Number of tokens consumed in reference span
    size_t trans_start_index = 0;             ///< Starting index in transcript word list
    size_t trans_word_count = 1;              ///< Number of tokens consumed in transcript span
    bool is_multi_word_phrase = false;        ///< True if match is a multi-word domain idiom/phrase
};

/**
 * @class IParaphraseMatcher
 * @brief Abstract interface defining synonym and semantic paraphrase equivalence algorithms.
 */
class IParaphraseMatcher {
public:
    virtual ~IParaphraseMatcher() = default;

    /**
     * @brief Retrieves the unique identifier of this matcher (e.g. "dictionary", "embedding", "hybrid").
     * @return std::string Matcher ID.
     */
    virtual std::string get_matcher_id() const = 0;

    /**
     * @brief Checks if two single words are direct synonyms.
     * @param word1 Reference word.
     * @param word2 Spoken/candidate word.
     * @param language Target language code ("es", "en").
     * @return bool True if words are synonyms, false otherwise.
     */
    virtual bool is_synonym(const std::string& word1, const std::string& word2, const std::string& language = "es") const = 0;

    /**
     * @brief Retrieves list of known synonyms for a given word.
     * @param word Target word.
     * @param language Target language code ("es", "en").
     * @return std::vector<std::string> Synonyms list.
     */
    virtual std::vector<std::string> get_synonyms(const std::string& word, const std::string& language = "es") const = 0;

    /**
     * @brief Compares two phrase spans for domain equivalence or synonymy.
     * @param ref_phrase Reference phrase span.
     * @param trans_phrase Spoken transcript phrase span.
     * @param domain_key Subject domain ("law", "economics", "science", "history", "general").
     * @param language Target language code.
     * @return ParaphraseMatchResult Match details and similarity rating.
     */
    virtual ParaphraseMatchResult compare_phrases(
        const std::string& ref_phrase,
        const std::string& trans_phrase,
        const std::string& domain_key = "general",
        const std::string& language = "es"
    ) const = 0;

    /**
     * @brief Scans reference and transcript word sequences for multi-word domain paraphrases and synonyms.
     * @param ref_words List of normalized reference words.
     * @param trans_words List of normalized transcript words.
     * @param domain_key Subject domain.
     * @param language Target language code.
     * @return std::vector<ParaphraseMatchResult> All detected paraphrase matches.
     */
    virtual std::vector<ParaphraseMatchResult> find_paraphrases(
        const std::vector<std::string>& ref_words,
        const std::vector<std::string>& trans_words,
        const std::string& domain_key = "general",
        const std::string& language = "es"
    ) const = 0;
};

#endif // I_PARAPHRASE_MATCHER_HPP
