/**
 * @file spanish_language_profile.hpp
 * @brief Spanish language profile implementation.
 */

#ifndef SPANISH_LANGUAGE_PROFILE_HPP
#define SPANISH_LANGUAGE_PROFILE_HPP

#include <unordered_set>
#include "primitives/i_language_profile.hpp"

/**
 * @class SpanishLanguageProfile
 * @brief Spanish-specific implementation of ILanguageProfile handling accent stripping, ¿/¡ punctuation removal, Spanish stopwords, and legal abbreviations.
 */
class SpanishLanguageProfile : public ILanguageProfile {
public:
    SpanishLanguageProfile();
    ~SpanishLanguageProfile() override = default;

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

#endif // SPANISH_LANGUAGE_PROFILE_HPP
