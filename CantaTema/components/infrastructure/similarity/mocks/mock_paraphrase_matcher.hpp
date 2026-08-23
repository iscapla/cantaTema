/**
 * @file mock_paraphrase_matcher.hpp
 * @brief GoogleMock implementation of IParaphraseMatcher.
 */

#ifndef MOCK_PARAPHRASE_MATCHER_HPP
#define MOCK_PARAPHRASE_MATCHER_HPP

#include <gmock/gmock.h>
#include "similarity/i_paraphrase_matcher.hpp"

class MockParaphraseMatcher : public IParaphraseMatcher {
public:
    MOCK_METHOD(std::string, get_matcher_id, (), (const, override));
    MOCK_METHOD(bool, is_synonym, (const std::string&, const std::string&, const std::string&), (const, override));
    MOCK_METHOD(std::vector<std::string>, get_synonyms, (const std::string&, const std::string&), (const, override));
    MOCK_METHOD(ParaphraseMatchResult, compare_phrases, (const std::string&, const std::string&, const std::string&, const std::string&), (const, override));
    MOCK_METHOD(std::vector<ParaphraseMatchResult>, find_paraphrases, (const std::vector<std::string>&, const std::vector<std::string>&, const std::string&, const std::string&), (const, override));
};

#endif // MOCK_PARAPHRASE_MATCHER_HPP
