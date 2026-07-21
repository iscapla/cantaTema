#ifndef __IOPERATION_COVERAGE_HPP
#define __IOPERATION_COVERAGE_HPP

#include <memory>
#include <string>
#include "primitives/definitions.hpp"
#include "primitives/user.hpp"

class IOperationCoverage
{
public:
    virtual ~IOperationCoverage() = default;

    /**
     * @brief Orchestrates the full audio-to-PDF coverage analysis pipeline.
     * 
     * @param user Logged-in user requesting the analysis.
     * @param practice_id Practice event ID.
     * @param whisper_model Model override for Whisper (or empty/"AUTO" for default).
     * @param llama_model Model override for llama.cpp embeddings (or empty/"AUTO" for default).
     * @param similarity_threshold Cosine similarity threshold override (or <= 0.0f for default).
     * @param language Language override (or empty to inherit from Subject).
     * @param out_analysis_execution_id Output parameter for the generated execution UUID.
     * @return rst_code_e RST_OK on success, or error code.
     */
    virtual rst_code_e analyze_practice_coverage(
        const std::shared_ptr<const User>& user,
        int practice_id,
        const std::string& whisper_model,
        const std::string& llama_model,
        float similarity_threshold,
        const std::string& language,
        std::string& out_analysis_execution_id
    ) = 0;
};

#endif // __IOPERATION_COVERAGE_HPP
