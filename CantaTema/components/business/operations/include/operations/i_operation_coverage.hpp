#ifndef __IOPERATION_COVERAGE_HPP
#define __IOPERATION_COVERAGE_HPP

#include <memory>
#include <string>
#include <vector>
#include "primitives/definitions.hpp"
#include "primitives/user.hpp"
#include "primitives/user_configuration.hpp"
#include "primitives/analysis_task.hpp"

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

    /**
     * @brief Orchestrates the full audio-to-PDF coverage analysis pipeline using a UserConfiguration.
     */
    virtual rst_code_e analyze_practice_coverage(
        const std::shared_ptr<const User>& user,
        int practice_id,
        const UserConfiguration& config,
        std::string& out_analysis_execution_id
    ) = 0;

    /**
     * @brief Retrieves JSON list of all analysis executions for a given practice event.
     * @param practice_id Practice event identifier.
     * @param out_executions_list_json Output string receiving JSON array.
     * @return rst_code_e RST_OK on success, or error code.
     */
    virtual rst_code_e get_analysis_executions_for_practice(
        int practice_id,
        std::string& out_executions_list_json
    ) = 0;

    /**
     * @brief Retrieves detailed report and configuration JSON for a specific analysis execution ID.
     * @param execution_id Unique GUID string of execution analysis.
     * @param out_report_json Output string receiving report JSON payload.
     * @param out_config_json Output string receiving configuration snapshot JSON payload.
     * @return rst_code_e RST_OK on success, or error code.
     */
    virtual rst_code_e get_analysis_execution_details(
        const std::string& execution_id,
        std::string& out_report_json,
        std::string& out_config_json
    ) = 0;

    //-------------------------------------------------------------------------------------
    // Task Persistence Queries (for Scheduler)
    //-------------------------------------------------------------------------------------

    /**
     * @brief Saves a newly created analysis task to persistence.
     * @param task Analysis task entity.
     * @return rst_code_e RST_OK on success, or error code.
     */
    virtual rst_code_e save_analysis_task(const AnalysisTask& task) = 0;

    /**
     * @brief Updates an existing analysis task status, progress, or completion metrics.
     * @param task Analysis task entity.
     * @return rst_code_e RST_OK on success, or error code.
     */
    virtual rst_code_e update_analysis_task(const AnalysisTask& task) = 0;

    /**
     * @brief Retrieves an analysis task by its unique task ID.
     * @param task_id Unique GUID string of the task.
     * @param out_task Output parameter receiving the retrieved task.
     * @return rst_code_e RST_OK on success, TASK_NOT_FOUND or DB_FAIL.
     */
    virtual rst_code_e get_analysis_task_by_id(const std::string& task_id, AnalysisTask& out_task) = 0;

    /**
     * @brief Retrieves all analysis tasks submitted by a specific user.
     * @param user_id User ID.
     * @param out_tasks Output list receiving tasks.
     * @return rst_code_e RST_OK on success, or error code.
     */
    virtual rst_code_e get_analysis_tasks_by_user(unsigned int user_id, std::vector<AnalysisTask>& out_tasks) = 0;

    /**
     * @brief Retrieves all analysis tasks in the system across all users.
     * @param out_tasks Output list receiving tasks.
     * @return rst_code_e RST_OK on success, or error code.
     */
    virtual rst_code_e get_all_analysis_tasks(std::vector<AnalysisTask>& out_tasks) = 0;

    /**
     * @brief Finds an active (QUEUED or running) task for a given practice event.
     * @param practice_id Practice event ID.
     * @param out_task Output parameter receiving the active task if found.
     * @return rst_code_e RST_OK if found, TASK_NOT_FOUND if none active.
     */
    virtual rst_code_e get_active_analysis_task_for_practice(int practice_id, AnalysisTask& out_task) = 0;

    /**
     * @brief Identifies interrupted RUNNING tasks from a previous crash/shutdown and recovers them.
     * @param max_retries Maximum retries allowed.
     * @param out_recovered_tasks Output list of tasks updated/recovered.
     * @return rst_code_e RST_OK on success, or error code.
     */
    virtual rst_code_e recover_interrupted_analysis_tasks(int max_retries, std::vector<AnalysisTask>& out_recovered_tasks) = 0;

    /**
     * @brief Deletes an analysis task from persistence.
     * @param task_id Unique GUID string of the task.
     * @return rst_code_e RST_OK on success, or error code.
     */
    virtual rst_code_e delete_analysis_task(const std::string& task_id) = 0;
};

#endif // __IOPERATION_COVERAGE_HPP
