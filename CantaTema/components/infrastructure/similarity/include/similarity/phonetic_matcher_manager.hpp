/**
 * @file phonetic_matcher_manager.hpp
 * @brief Singleton factory and registry for phonetic matchers.
 */

#ifndef PHONETIC_MATCHER_MANAGER_HPP
#define PHONETIC_MATCHER_MANAGER_HPP

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include "similarity/i_phonetic_matcher.hpp"

/**
 * @class PhoneticMatcherManager
 * @brief Manages phonetic matcher instances, providing dynamic retrieval based on matcher IDs or system configuration.
 */
class PhoneticMatcherManager {
public:
    static PhoneticMatcherManager& getInstance();

    PhoneticMatcherManager(const PhoneticMatcherManager&) = delete;
    PhoneticMatcherManager& operator=(const PhoneticMatcherManager&) = delete;

    /**
     * @brief Registers a phonetic matcher into the manager.
     * @param matcher Shared pointer to IPhoneticMatcher implementation.
     */
    void register_matcher(std::shared_ptr<IPhoneticMatcher> matcher);

    /**
     * @brief Retrieves a matcher by its matcher ID (e.g. "double_metaphone", "soundex").
     * @param matcher_id Matcher ID. If unknown, falls back to "double_metaphone".
     * @return std::shared_ptr<IPhoneticMatcher> Resolved matcher instance.
     */
    std::shared_ptr<IPhoneticMatcher> get_matcher(const std::string& matcher_id);

    /**
     * @brief Retrieves the active phonetic matcher configured in system.ini.
     * @return std::shared_ptr<IPhoneticMatcher> Active matcher instance.
     */
    std::shared_ptr<IPhoneticMatcher> get_active_matcher();

private:
    PhoneticMatcherManager();
    ~PhoneticMatcherManager() = default;

    std::unordered_map<std::string, std::shared_ptr<IPhoneticMatcher>> m_matchers;
    mutable std::mutex m_mutex;
};

#endif // PHONETIC_MATCHER_MANAGER_HPP
