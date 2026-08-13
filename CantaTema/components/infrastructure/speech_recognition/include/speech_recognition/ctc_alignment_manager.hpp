/**
 * @file ctc_alignment_manager.hpp
 * @brief Singleton factory and registry for CTC forced aligners.
 */

#ifndef CTC_ALIGNMENT_MANAGER_HPP
#define CTC_ALIGNMENT_MANAGER_HPP

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include "speech_recognition/i_ctc_aligner.hpp"

/**
 * @class CtcAlignmentManager
 * @brief Manages CTC aligner instances, providing dynamic retrieval based on aligner IDs or system configuration.
 */
class CtcAlignmentManager {
public:
    static CtcAlignmentManager& getInstance();

    CtcAlignmentManager(const CtcAlignmentManager&) = delete;
    CtcAlignmentManager& operator=(const CtcAlignmentManager&) = delete;

    /**
     * @brief Registers a CTC aligner into the manager.
     * @param aligner Shared pointer to ICtcAligner implementation.
     */
    void register_aligner(std::shared_ptr<ICtcAligner> aligner);

    /**
     * @brief Retrieves an aligner by its aligner ID (e.g. "whisper_ctc").
     * @param aligner_id Aligner ID. If unknown or "AUTO", falls back to "whisper_ctc".
     * @return std::shared_ptr<ICtcAligner> Resolved aligner instance.
     */
    std::shared_ptr<ICtcAligner> get_aligner(const std::string& aligner_id);

    /**
     * @brief Retrieves the active aligner configured in system.ini.
     * @return std::shared_ptr<ICtcAligner> Active aligner instance.
     */
    std::shared_ptr<ICtcAligner> get_active_aligner();

private:
    CtcAlignmentManager();
    ~CtcAlignmentManager() = default;

    std::unordered_map<std::string, std::shared_ptr<ICtcAligner>> m_aligners;
    mutable std::mutex m_mutex;
};

#endif // CTC_ALIGNMENT_MANAGER_HPP
