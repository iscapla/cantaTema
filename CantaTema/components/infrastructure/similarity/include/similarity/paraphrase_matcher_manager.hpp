/**
 * @file paraphrase_matcher_manager.hpp
 * @brief Singleton manager providing access to configured semantic paraphrase matcher instances.
 */

#ifndef PARAPHRASE_MATCHER_MANAGER_HPP
#define PARAPHRASE_MATCHER_MANAGER_HPP

#include "similarity/i_paraphrase_matcher.hpp"
#include "similarity/dictionary_paraphrase_matcher.hpp"
#include "similarity/embedding_paraphrase_matcher.hpp"
#include "similarity/hybrid_paraphrase_matcher.hpp"
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

/**
 * @class ParaphraseMatcherManager
 * @brief Singleton registry for semantic paraphrase matchers.
 */
class ParaphraseMatcherManager {
public:
    static ParaphraseMatcherManager& getInstance();

    /**
     * @brief Retrieves a matcher by algorithm ID ("dictionary", "embedding", "hybrid").
     */
    std::shared_ptr<IParaphraseMatcher> get_matcher(const std::string& matcher_id);

    /**
     * @brief Retrieves the active matcher determined by system.ini configuration.
     */
    std::shared_ptr<IParaphraseMatcher> get_active_matcher();

    /**
     * @brief Sets the embedding engine used across embedding and hybrid matchers.
     */
    void set_embedding_engine(std::shared_ptr<IEmbeddingEngine> engine);

private:
    ParaphraseMatcherManager();
    ~ParaphraseMatcherManager() = default;
    ParaphraseMatcherManager(const ParaphraseMatcherManager&) = delete;
    ParaphraseMatcherManager& operator=(const ParaphraseMatcherManager&) = delete;

    std::mutex m_mutex;
    std::unordered_map<std::string, std::shared_ptr<IParaphraseMatcher>> m_matchers;
    std::shared_ptr<IEmbeddingEngine> m_embedding_engine;
};

#endif // PARAPHRASE_MATCHER_MANAGER_HPP
