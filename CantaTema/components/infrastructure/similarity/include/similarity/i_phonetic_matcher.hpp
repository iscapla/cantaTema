/**
 * @file i_phonetic_matcher.hpp
 * @brief Abstract interface for phonetic & ASR noise compensation matchers.
 */

#ifndef I_PHONETIC_MATCHER_HPP
#define I_PHONETIC_MATCHER_HPP

#include <string>

/**
 * @struct PhoneticMatchResult
 * @brief Result of comparing two tokens for phonetic sound similarity.
 */
struct PhoneticMatchResult {
    bool is_match = false;               ///< True if tokens are phonetically equivalent
    float similarity_score = 0.0f;       ///< Phonetic similarity score [0.0 - 1.0]
    bool is_minor_mispronunciation = false; ///< True if tokens differ orthographically but match phonetically
    std::string primary_code;            ///< Primary phonetic sound key
    std::string secondary_code;          ///< Secondary phonetic sound key (if applicable)
};

/**
 * @class IPhoneticMatcher
 * @brief Abstract interface defining phonetic sound key generation and token comparison algorithms.
 */
class IPhoneticMatcher {
public:
    virtual ~IPhoneticMatcher() = default;

    /**
     * @brief Retrieves the unique identifier of this matcher (e.g. "double_metaphone", "soundex").
     * @return std::string Matcher ID.
     */
    virtual std::string get_matcher_id() const = 0;

    /**
     * @brief Generates the primary phonetic sound code for a given word.
     * @param word Token or word.
     * @return std::string Phonetic sound key.
     */
    virtual std::string get_phonetic_code(const std::string& word) const = 0;

    /**
     * @brief Compares two words phonetically and returns a detailed match breakdown.
     * @param word1 Reference word.
     * @param word2 Spoken/candidate word.
     * @return PhoneticMatchResult Phonetic match status and score.
     */
    virtual PhoneticMatchResult compare_words(const std::string& word1, const std::string& word2) const = 0;
};

#endif // I_PHONETIC_MATCHER_HPP
