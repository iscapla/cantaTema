/**
 * @file language_profile_manager.hpp
 * @brief Singleton factory and registry for language profiles.
 */

#ifndef LANGUAGE_PROFILE_MANAGER_HPP
#define LANGUAGE_PROFILE_MANAGER_HPP

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include "primitives/i_language_profile.hpp"

/**
 * @class LanguageProfileManager
 * @brief Manages language profile instances, providing dynamic retrieval based on ISO language codes or system configuration.
 */
class LanguageProfileManager {
public:
    static LanguageProfileManager& getInstance();

    LanguageProfileManager(const LanguageProfileManager&) = delete;
    LanguageProfileManager& operator=(const LanguageProfileManager&) = delete;

    /**
     * @brief Registers a language profile into the manager.
     * @param profile Shared pointer to the ILanguageProfile implementation.
     */
    void register_profile(std::shared_ptr<ILanguageProfile> profile);

    /**
     * @brief Retrieves a language profile by its language code (e.g. "es", "en").
     * @param language_code ISO code. If unknown, falls back to Spanish ("es").
     * @return std::shared_ptr<ILanguageProfile> Resolved profile instance.
     */
    std::shared_ptr<ILanguageProfile> get_profile(const std::string& language_code);

    /**
     * @brief Retrieves the active language profile configured in system.ini.
     * @return std::shared_ptr<ILanguageProfile> Active profile instance.
     */
    std::shared_ptr<ILanguageProfile> get_active_profile();

private:
    LanguageProfileManager();
    ~LanguageProfileManager() = default;

    std::unordered_map<std::string, std::shared_ptr<ILanguageProfile>> m_profiles;
    mutable std::mutex m_mutex;
};

#endif // LANGUAGE_PROFILE_MANAGER_HPP
