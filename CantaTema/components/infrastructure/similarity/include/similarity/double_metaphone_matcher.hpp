/**
 * @file double_metaphone_matcher.hpp
 * @brief Double Metaphone phonetic matcher implementation.
 */

#ifndef DOUBLE_METAPHONE_MATCHER_HPP
#define DOUBLE_METAPHONE_MATCHER_HPP

#include "similarity/i_phonetic_matcher.hpp"

/**
 * @class DoubleMetaphoneMatcher
 * @brief Implementation of IPhoneticMatcher using the Double Metaphone algorithm for Spanish and English phonetic equivalence.
 */
class DoubleMetaphoneMatcher : public IPhoneticMatcher {
public:
    DoubleMetaphoneMatcher() = default;
    ~DoubleMetaphoneMatcher() override = default;

    std::string get_matcher_id() const override;
    std::string get_phonetic_code(const std::string& word) const override;
    PhoneticMatchResult compare_words(const std::string& word1, const std::string& word2) const override;

private:
    std::pair<std::string, std::string> compute_double_metaphone(const std::string& word) const;
};

#endif // DOUBLE_METAPHONE_MATCHER_HPP
