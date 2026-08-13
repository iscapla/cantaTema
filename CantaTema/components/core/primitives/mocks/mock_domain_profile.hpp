/**
 * @file mock_domain_profile.hpp
 * @brief GoogleMock implementation of IDomainProfile interface for unit testing.
 */

#ifndef MOCK_DOMAIN_PROFILE_HPP
#define MOCK_DOMAIN_PROFILE_HPP

#include <gmock/gmock.h>
#include "primitives/i_domain_profile.hpp"

class MockDomainProfile : public IDomainProfile {
public:
    MOCK_METHOD(std::string, get_domain_key, (), (const, override));
    MOCK_METHOD(std::string, get_domain_name, (), (const, override));
    MOCK_METHOD(float, get_token_weight, (const std::string& token, bool is_stopword), (const, override));
    MOCK_METHOD(bool, is_high_priority_citation, (const std::string& token), (const, override));
    MOCK_METHOD(std::string, get_missing_warning_badge, (), (const, override));
};

#endif // MOCK_DOMAIN_PROFILE_HPP
