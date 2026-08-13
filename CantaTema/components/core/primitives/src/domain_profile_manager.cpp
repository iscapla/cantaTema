/**
 * @file domain_profile_manager.cpp
 * @brief Implementation of DomainProfileManager.
 */

#include "primitives/domain_profile_manager.hpp"
#include "primitives/domain_profiles.hpp"
#include "configuration/configuration_system.hpp"
#include <algorithm>

DomainProfileManager& DomainProfileManager::getInstance() {
    static DomainProfileManager instance;
    return instance;
}

DomainProfileManager::DomainProfileManager() {
    register_profile(std::make_shared<LawDomainProfile>());
    register_profile(std::make_shared<EconomicsDomainProfile>());
    register_profile(std::make_shared<HistoryDomainProfile>());
    register_profile(std::make_shared<ScienceDomainProfile>());
    register_profile(std::make_shared<GeneralDomainProfile>());
}

void DomainProfileManager::register_profile(std::shared_ptr<IDomainProfile> profile) {
    if (!profile) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_profiles[profile->get_domain_key()] = profile;
}

std::shared_ptr<IDomainProfile> DomainProfileManager::get_profile(const std::string& domain_key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string key = domain_key;
    std::transform(key.begin(), key.end(), key.begin(), ::tolower);

    auto it = m_profiles.find(key);
    if (it != m_profiles.end()) {
        return it->second;
    }

    // Default fallback to "general" if unregistered
    auto default_it = m_profiles.find("general");
    if (default_it != m_profiles.end()) {
        return default_it->second;
    }

    return nullptr;
}

std::shared_ptr<IDomainProfile> DomainProfileManager::get_active_profile() {
    std::string active_domain = ConfigurationSystem::getInstance().get_comparison_active_domain();
    return get_profile(active_domain);
}
