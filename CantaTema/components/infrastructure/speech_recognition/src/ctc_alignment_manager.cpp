/**
 * @file ctc_alignment_manager.cpp
 * @brief Implementation of CtcAlignmentManager.
 */

#include "speech_recognition/ctc_alignment_manager.hpp"
#include "speech_recognition/whisper_ctc_aligner.hpp"
#include "configuration/configuration_system.hpp"
#include <algorithm>

CtcAlignmentManager& CtcAlignmentManager::getInstance() {
    static CtcAlignmentManager instance;
    return instance;
}

CtcAlignmentManager::CtcAlignmentManager() {
    register_aligner(std::make_shared<WhisperCtcAligner>());
}

void CtcAlignmentManager::register_aligner(std::shared_ptr<ICtcAligner> aligner) {
    if (!aligner) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_aligners[aligner->get_aligner_id()] = aligner;
}

std::shared_ptr<ICtcAligner> CtcAlignmentManager::get_aligner(const std::string& aligner_id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string id = aligner_id;
    std::transform(id.begin(), id.end(), id.begin(), ::tolower);

    auto it = m_aligners.find(id);
    if (it != m_aligners.end()) {
        return it->second;
    }

    // Fallback to default "whisper_ctc"
    auto default_it = m_aligners.find("whisper_ctc");
    if (default_it != m_aligners.end()) {
        return default_it->second;
    }

    return nullptr;
}

std::shared_ptr<ICtcAligner> CtcAlignmentManager::get_active_aligner() {
    std::string mode = ConfigurationSystem::getInstance().get_alignment_mode();
    return get_aligner(mode);
}
