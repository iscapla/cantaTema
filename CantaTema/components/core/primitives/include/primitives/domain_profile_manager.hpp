/**
 * @file domain_profile_manager.hpp
 * @brief Singleton factory and registry for subject domain profiles.
 */

#ifndef DOMAIN_PROFILE_MANAGER_HPP
#define DOMAIN_PROFILE_MANAGER_HPP

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include "primitives/i_domain_profile.hpp"

/**
 * @class DomainProfileManager
 * @brief Manages domain profile instances, providing dynamic retrieval based on domain keys or system configuration.
 */
class DomainProfileManager {
public:
    static DomainProfileManager& getInstance();

    DomainProfileManager(const DomainProfileManager&) = delete;
    DomainProfileManager& operator=(const DomainProfileManager&) = delete;

    /**
     * @brief Registers a subject domain profile into the manager.
     * @param profile Shared pointer to the IDomainProfile implementation.
     */
    void register_profile(std::shared_ptr<IDomainProfile> profile);

    /**
     * @brief Retrieves a domain profile by its domain key (e.g. "law", "economics", "history", "science", "general").
     * @param domain_key Domain key. If unknown, falls back to "general".
     * @return std::shared_ptr<IDomainProfile> Resolved profile instance.
     */
    std::shared_ptr<IDomainProfile> get_profile(const std::string& domain_key);

    /**
     * @brief Retrieves the active domain profile configured in system.ini.
     * @return std::shared_ptr<IDomainProfile> Active domain profile instance.
     */
    std::shared_ptr<IDomainProfile> get_active_profile();

private:
    DomainProfileManager();
    ~DomainProfileManager() = default;

    std::unordered_map<std::string, std::shared_ptr<IDomainProfile>> m_profiles;
    mutable std::mutex m_mutex;
};

#endif // DOMAIN_PROFILE_MANAGER_HPP
