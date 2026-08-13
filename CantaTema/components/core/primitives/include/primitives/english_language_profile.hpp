/**
 * @file english_language_profile.hpp
 * @brief English language profile implementation.
 */

#ifndef ENGLISH_LANGUAGE_PROFILE_HPP
#define ENGLISH_LANGUAGE_PROFILE_HPP

#include <unordered_set>
#include "primitives/i_language_profile.hpp"

/**
 * @class EnglishLanguageProfile
 * @brief English-specific implementation of ILanguageProfile handling normalization, English stopwords, and common abbreviations.
 */
class EnglishLanguageProfile : public ILanguageProfile {
public:
    EnglishLanguageProfile();
    ~EnglishLanguageProfile() override = default;

    std::string get_language_code() const override;
    std::string normalize_word(const std::string& input) const override;
    bool is_stopword(const std::string& word) const override;
    bool is_abbreviation(const std::string& word) const override;
    std::vector<std::string> get_stopwords() const override;
    std::vector<std::string> get_abbreviations() const override;

private:
    std::unordered_set<std::string> m_stopwords;
    std::unordered_set<std::string> m_abbreviations;
};

#endif // ENGLISH_LANGUAGE_PROFILE_HPP
