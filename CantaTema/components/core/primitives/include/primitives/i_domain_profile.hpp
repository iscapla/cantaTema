/**
 * @file i_domain_profile.hpp
 * @brief Abstract interface for academic subject domain profiles.
 */

#ifndef I_DOMAIN_PROFILE_HPP
#define I_DOMAIN_PROFILE_HPP

#include <string>

/**
 * @class IDomainProfile
 * @brief Abstract interface defining specialized evaluation rules, token weight multipliers, high-priority citation detection, and custom warning badges per subject domain.
 */
class IDomainProfile {
public:
    virtual ~IDomainProfile() = default;

    /**
     * @brief Retrieves the unique key identifying the domain (e.g. "law", "economics", "history", "science", "general").
     * @return std::string Domain key.
     */
    virtual std::string get_domain_key() const = 0;

    /**
     * @brief Retrieves the display name of the subject domain.
     * @return std::string Display name.
     */
    virtual std::string get_domain_name() const = 0;

    /**
     * @brief Computes the linguistic importance weight multiplier for a token under this subject domain.
     * @param token Token or entity string.
     * @param is_stopword True if the token is a grammatical stopword.
     * @return float Weight multiplier.
     */
    virtual float get_token_weight(const std::string& token, bool is_stopword) const = 0;

    /**
     * @brief Checks if a token represents a high-priority citation or indicator for this domain.
     * @param token Token or entity string.
     * @return bool True if high priority citation, false otherwise.
     */
    virtual bool is_high_priority_citation(const std::string& token) const = 0;

    /**
     * @brief Retrieves the custom warning badge text displayed when key domain terms are omitted.
     * @return std::string Warning badge message.
     */
    virtual std::string get_missing_warning_badge() const = 0;
};

#endif // I_DOMAIN_PROFILE_HPP
