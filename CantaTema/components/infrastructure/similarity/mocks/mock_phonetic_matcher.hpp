/**
 * @file mock_phonetic_matcher.hpp
 * @brief GoogleMock implementation of IPhoneticMatcher interface for unit testing.
 */

#ifndef MOCK_PHONETIC_MATCHER_HPP
#define MOCK_PHONETIC_MATCHER_HPP

#include <gmock/gmock.h>
#include "similarity/i_phonetic_matcher.hpp"

class MockPhoneticMatcher : public IPhoneticMatcher {
public:
    MOCK_METHOD(std::string, get_matcher_id, (), (const, override));
    MOCK_METHOD(std::string, get_phonetic_code, (const std::string& word), (const, override));
    MOCK_METHOD(PhoneticMatchResult, compare_words, (const std::string& word1, const std::string& word2), (const, override));
};

#endif // MOCK_PHONETIC_MATCHER_HPP
