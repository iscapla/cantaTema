/**
 * @file soundex_matcher.hpp
 * @brief Soundex phonetic matcher implementation.
 */

#ifndef SOUNDEX_MATCHER_HPP
#define SOUNDEX_MATCHER_HPP

#include "similarity/i_phonetic_matcher.hpp"

/**
 * @class SoundexMatcher
 * @brief Implementation of IPhoneticMatcher using standard Soundex phonetic encoding.
 */
class SoundexMatcher : public IPhoneticMatcher {
public:
    SoundexMatcher() = default;
    ~SoundexMatcher() override = default;

    std::string get_matcher_id() const override;
    std::string get_phonetic_code(const std::string& word) const override;
    PhoneticMatchResult compare_words(const std::string& word1, const std::string& word2) const override;
};

#endif // SOUNDEX_MATCHER_HPP
