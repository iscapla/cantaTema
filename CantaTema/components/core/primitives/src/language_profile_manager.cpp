/**
 * @file language_profile_manager.cpp
 * @brief Implementation of LanguageProfileManager.
 */

#include "primitives/language_profile_manager.hpp"
#include "primitives/spanish_language_profile.hpp"
#include "primitives/english_language_profile.hpp"
#include "configuration/configuration_system.hpp"

LanguageProfileManager& LanguageProfileManager::getInstance() {
    static LanguageProfileManager instance;
    return instance;
}

LanguageProfileManager::LanguageProfileManager() {
    register_profile(std::make_shared<SpanishLanguageProfile>());
    register_profile(std::make_shared<EnglishLanguageProfile>());
}

void LanguageProfileManager::register_profile(std::shared_ptr<ILanguageProfile> profile) {
    if (!profile) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_profiles[profile->get_language_code()] = profile;
}

std::shared_ptr<ILanguageProfile> LanguageProfileManager::get_profile(const std::string& language_code) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string code = language_code;
    std::transform(code.begin(), code.end(), code.begin(), ::tolower);

    auto it = m_profiles.find(code);
    if (it != m_profiles.end()) {
        return it->second;
    }

    // Default fallback to "es" if unregistered
    auto default_it = m_profiles.find("es");
    if (default_it != m_profiles.end()) {
        return default_it->second;
    }

    return nullptr;
}

std::shared_ptr<ILanguageProfile> LanguageProfileManager::get_active_profile() {
    std::string active_lang = ConfigurationSystem::getInstance().get_comparison_active_language();
    return get_profile(active_lang);
}
