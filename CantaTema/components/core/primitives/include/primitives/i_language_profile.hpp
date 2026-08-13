/**
 * @file i_language_profile.hpp
 * @brief Abstract interface for multilingual language profiles.
 */

#ifndef I_LANGUAGE_PROFILE_HPP
#define I_LANGUAGE_PROFILE_HPP

#include <string>
#include <vector>

/**
 * @class ILanguageProfile
 * @brief Abstract interface for language-specific text processing, UTF-8 normalization, stopword lookups, and abbreviation handling.
 */
class ILanguageProfile {
public:
    virtual ~ILanguageProfile() = default;

    /**
     * @brief Retrieves the ISO language code for this profile (e.g., "es", "en").
     * @return std::string Language code.
     */
    virtual std::string get_language_code() const = 0;

    /**
     * @brief Normalizes a input word by lowercasing, stripping accents, and removing non-alphanumeric punctuation.
     * @param input Raw token or word.
     * @return std::string Normalized token.
     */
    virtual std::string normalize_word(const std::string& input) const = 0;

    /**
     * @brief Checks if a given word is classified as a grammatical stopword in this language.
     * @param word Token to check (case-insensitive or normalized).
     * @return bool True if stopword, false otherwise.
     */
    virtual bool is_stopword(const std::string& word) const = 0;

    /**
     * @brief Checks if a given word is a standard abbreviation in this language (e.g. "art.", "sec.").
     * @param word Token to check.
     * @return bool True if abbreviation, false otherwise.
     */
    virtual bool is_abbreviation(const std::string& word) const = 0;

    /**
     * @brief Retrieves the complete set of stopwords registered for this language.
     * @return std::vector<std::string> List of stopwords.
     */
    virtual std::vector<std::string> get_stopwords() const = 0;

    /**
     * @brief Retrieves the complete set of abbreviations registered for this language.
     * @return std::vector<std::string> List of abbreviations.
     */
    virtual std::vector<std::string> get_abbreviations() const = 0;
};

#endif // I_LANGUAGE_PROFILE_HPP
