/**
 * @file mock_language_profile.hpp
 * @brief GoogleMock implementation of ILanguageProfile interface for unit testing.
 */

#ifndef MOCK_LANGUAGE_PROFILE_HPP
#define MOCK_LANGUAGE_PROFILE_HPP

#include <gmock/gmock.h>
#include "primitives/i_language_profile.hpp"

class MockLanguageProfile : public ILanguageProfile {
public:
    MOCK_METHOD(std::string, get_language_code, (), (const, override));
    MOCK_METHOD(std::string, normalize_word, (const std::string& input), (const, override));
    MOCK_METHOD(bool, is_stopword, (const std::string& word), (const, override));
    MOCK_METHOD(bool, is_abbreviation, (const std::string& word), (const, override));
    MOCK_METHOD(std::vector<std::string>, get_stopwords, (), (const, override));
    MOCK_METHOD(std::vector<std::string>, get_abbreviations, (), (const, override));
};

#endif // MOCK_LANGUAGE_PROFILE_HPP
