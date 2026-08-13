/**
 * @file phonetic_matcher_manager.cpp
 * @brief Implementation of PhoneticMatcherManager.
 */

#include "similarity/phonetic_matcher_manager.hpp"
#include "similarity/double_metaphone_matcher.hpp"
#include "similarity/soundex_matcher.hpp"
#include "configuration/configuration_system.hpp"
#include <algorithm>

PhoneticMatcherManager& PhoneticMatcherManager::getInstance() {
    static PhoneticMatcherManager instance;
    return instance;
}

PhoneticMatcherManager::PhoneticMatcherManager() {
    register_matcher(std::make_shared<DoubleMetaphoneMatcher>());
    register_matcher(std::make_shared<SoundexMatcher>());
}

void PhoneticMatcherManager::register_matcher(std::shared_ptr<IPhoneticMatcher> matcher) {
    if (!matcher) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_matchers[matcher->get_matcher_id()] = matcher;
}

std::shared_ptr<IPhoneticMatcher> PhoneticMatcherManager::get_matcher(const std::string& matcher_id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string id = matcher_id;
    std::transform(id.begin(), id.end(), id.begin(), ::tolower);

    auto it = m_matchers.find(id);
    if (it != m_matchers.end()) {
        return it->second;
    }

    // Default fallback to "double_metaphone"
    auto default_it = m_matchers.find("double_metaphone");
    if (default_it != m_matchers.end()) {
        return default_it->second;
    }

    return nullptr;
}

std::shared_ptr<IPhoneticMatcher> PhoneticMatcherManager::get_active_matcher() {
    std::string default_matcher = ConfigurationSystem::getInstance().get_phonetic_default_matcher();
    return get_matcher(default_matcher);
}
