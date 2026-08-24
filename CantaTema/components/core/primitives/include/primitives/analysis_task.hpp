/**
 * @file analysis_task.hpp
 * @brief Domain entity representing an asynchronous analysis task and its execution lifecycle.
 */

#ifndef __ANALYSIS_TASK_HPP
#define __ANALYSIS_TASK_HPP

#include <string>
#include <ctime>
#include "primitives/definitions.hpp"

/**
 * @enum AnalysisTaskStatus
 * @brief Represents the lifecycle status of an asynchronous analysis task.
 */
enum class AnalysisTaskStatus
{
    QUEUED = 0,
    CONVERTING_AUDIO,
    TRANSCRIBING,
    GENERATING_EMBEDDINGS,
    MATCHING_SIMILARITY,
    COMPLETED,
    FAILED,
    CANCELLED
};

/**
 * @class AnalysisTask
 * @brief Represents a scheduled analysis job submitted by a user for background execution.
 */
class AnalysisTask
{
public:
    /**
     * @brief Default constructor initializing default task properties.
     */
    AnalysisTask();

    /**
     * @brief Parameterized constructor for creating a new queued task.
     * @param task_id Unique GUID string identifying the task.
     * @param user_id ID of the user submitting the task.
     * @param practice_id ID of the practice event being analyzed.
     * @param config_snapshot_json JSON snapshot of user configuration.
     */
    AnalysisTask(
        std::string task_id,
        unsigned int user_id,
        int practice_id,
        std::string config_snapshot_json = ""
    );

    ~AnalysisTask() = default;

    // Getters
    const std::string& get_task_id() const;
    unsigned int get_user_id() const;
    int get_practice_id() const;
    AnalysisTaskStatus get_status() const;
    int get_progress_percentage() const;
    const std::string& get_stage_description() const;
    const std::string& get_error_message() const;
    rst_code_e get_result_code() const;
    const std::string& get_execution_id() const;
    const std::string& get_config_snapshot_json() const;
    std::time_t get_created_at() const;
    std::time_t get_started_at() const;
    std::time_t get_completed_at() const;
    int get_retry_count() const;

    // Setters
    void set_task_id(std::string task_id);
    void set_user_id(unsigned int user_id);
    void set_practice_id(int practice_id);
    void set_status(AnalysisTaskStatus status);
    void set_progress_percentage(int progress);
    void set_stage_description(std::string desc);
    void set_error_message(std::string err);
    void set_result_code(rst_code_e code);
    void set_execution_id(std::string execution_id);
    void set_config_snapshot_json(std::string config_json);
    void set_created_at(std::time_t created);
    void set_started_at(std::time_t started);
    void set_completed_at(std::time_t completed);
    void set_retry_count(int count);

    // Helpers
    static std::string status_to_string(AnalysisTaskStatus status);
    static AnalysisTaskStatus string_to_status(const std::string& status_str);

    /**
     * @brief Checks if task is in a terminal state (COMPLETED, FAILED, or CANCELLED).
     */
    bool is_finished() const;

    /**
     * @brief Checks if task is actively executing in one of the active processing stages.
     */
    bool is_running() const;

    /**
     * @brief Serializes the task details into a JSON formatted string.
     */
    std::string to_json() const;

private:
    std::string m_task_id;
    unsigned int m_user_id{0};
    int m_practice_id{0};
    AnalysisTaskStatus m_status{AnalysisTaskStatus::QUEUED};
    int m_progress_percentage{0};
    std::string m_stage_description{"Queued in scheduler"};
    std::string m_error_message;
    rst_code_e m_result_code{RST_OK};
    std::string m_execution_id;
    std::string m_config_snapshot_json;
    std::time_t m_created_at{0};
    std::time_t m_started_at{0};
    std::time_t m_completed_at{0};
    int m_retry_count{0};
};

#endif // __ANALYSIS_TASK_HPP
