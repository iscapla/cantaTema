/**
 * @file paraphrase_matcher_manager.cpp
 * @brief Implementation of ParaphraseMatcherManager singleton.
 */

#include "similarity/paraphrase_matcher_manager.hpp"
#include "configuration/configuration_system.hpp"

ParaphraseMatcherManager& ParaphraseMatcherManager::getInstance() {
    static ParaphraseMatcherManager instance;
    return instance;
}

ParaphraseMatcherManager::ParaphraseMatcherManager() {
    auto dict = std::make_shared<DictionaryParaphraseMatcher>();
    auto embed = std::make_shared<EmbeddingParaphraseMatcher>();
    auto hybrid = std::make_shared<HybridParaphraseMatcher>();

    m_matchers["dictionary"] = dict;
    m_matchers["embedding"] = embed;
    m_matchers["hybrid"] = hybrid;
}

void ParaphraseMatcherManager::set_embedding_engine(std::shared_ptr<IEmbeddingEngine> engine) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_embedding_engine = engine;

    auto embed_it = m_matchers.find("embedding");
    if (embed_it != m_matchers.end()) {
        auto embed = std::dynamic_pointer_cast<EmbeddingParaphraseMatcher>(embed_it->second);
        if (embed) embed->set_embedding_engine(engine);
    }

    auto hybrid_it = m_matchers.find("hybrid");
    if (hybrid_it != m_matchers.end()) {
        auto hybrid = std::dynamic_pointer_cast<HybridParaphraseMatcher>(hybrid_it->second);
        if (hybrid) hybrid->set_embedding_engine(engine);
    }
}

std::shared_ptr<IParaphraseMatcher> ParaphraseMatcherManager::get_matcher(const std::string& matcher_id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_matchers.find(matcher_id);
    if (it != m_matchers.end()) {
        return it->second;
    }
    // Default fallback is hybrid
    return m_matchers["hybrid"];
}

std::shared_ptr<IParaphraseMatcher> ParaphraseMatcherManager::get_active_matcher() {
    std::string default_matcher = ConfigurationSystem::getInstance().get_semantic_paraphrase_default_matcher();
    if (default_matcher.empty()) {
        default_matcher = "hybrid";
    }
    return get_matcher(default_matcher);
}
