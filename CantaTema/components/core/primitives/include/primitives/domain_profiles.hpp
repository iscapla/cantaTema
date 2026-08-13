/**
 * @file domain_profiles.hpp
 * @brief Concrete subject domain profile implementations (Law, Economics, History, Science, General).
 */

#ifndef DOMAIN_PROFILES_HPP
#define DOMAIN_PROFILES_HPP

#include <unordered_set>
#include "primitives/i_domain_profile.hpp"

/**
 * @class LawDomainProfile
 * @brief Law domain profile prioritizing legal citations (Article, Law, RD) with 4.0x weight.
 */
class LawDomainProfile : public IDomainProfile {
public:
    LawDomainProfile();
    ~LawDomainProfile() override = default;

    std::string get_domain_key() const override;
    std::string get_domain_name() const override;
    float get_token_weight(const std::string& token, bool is_stopword) const override;
    bool is_high_priority_citation(const std::string& token) const override;
    std::string get_missing_warning_badge() const override;

private:
    std::unordered_set<std::string> m_citation_keywords;
};

/**
 * @class EconomicsDomainProfile
 * @brief Economics domain profile prioritizing financial indicators (GDP/PIB, IPC, inflation, currency, %) with 4.0x weight.
 */
class EconomicsDomainProfile : public IDomainProfile {
public:
    EconomicsDomainProfile();
    ~EconomicsDomainProfile() override = default;

    std::string get_domain_key() const override;
    std::string get_domain_name() const override;
    float get_token_weight(const std::string& token, bool is_stopword) const override;
    bool is_high_priority_citation(const std::string& token) const override;
    std::string get_missing_warning_badge() const override;

private:
    std::unordered_set<std::string> m_economic_indicators;
};

/**
 * @class HistoryDomainProfile
 * @brief History domain profile prioritizing dates, centuries (Siglo XX), treaties, and historical eras with 4.0x weight.
 */
class HistoryDomainProfile : public IDomainProfile {
public:
    HistoryDomainProfile();
    ~HistoryDomainProfile() override = default;

    std::string get_domain_key() const override;
    std::string get_domain_name() const override;
    float get_token_weight(const std::string& token, bool is_stopword) const override;
    bool is_high_priority_citation(const std::string& token) const override;
    std::string get_missing_warning_badge() const override;

private:
    std::unordered_set<std::string> m_history_keywords;
};

/**
 * @class ScienceDomainProfile
 * @brief Science domain profile prioritizing scientific terms (DNA, RNA, ATP), SI units (kg, Hz, mol), and formulas with 4.0x weight.
 */
class ScienceDomainProfile : public IDomainProfile {
public:
    ScienceDomainProfile();
    ~ScienceDomainProfile() override = default;

    std::string get_domain_key() const override;
    std::string get_domain_name() const override;
    float get_token_weight(const std::string& token, bool is_stopword) const override;
    bool is_high_priority_citation(const std::string& token) const override;
    std::string get_missing_warning_badge() const override;

private:
    std::unordered_set<std::string> m_science_terms;
    std::unordered_set<std::string> m_si_units;
};

/**
 * @class GeneralDomainProfile
 * @brief General subject domain profile for literature, essays, and general academic content.
 */
class GeneralDomainProfile : public IDomainProfile {
public:
    GeneralDomainProfile();
    ~GeneralDomainProfile() override = default;

    std::string get_domain_key() const override;
    std::string get_domain_name() const override;
    float get_token_weight(const std::string& token, bool is_stopword) const override;
    bool is_high_priority_citation(const std::string& token) const override;
    std::string get_missing_warning_badge() const override;
};

#endif // DOMAIN_PROFILES_HPP
