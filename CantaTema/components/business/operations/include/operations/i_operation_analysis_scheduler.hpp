/**
 * @file i_operation_analysis_scheduler.hpp
 * @brief Interface for managing background analysis tasks, execution queues, and scheduling.
 */

#ifndef __I_OPERATION_ANALYSIS_SCHEDULER_HPP
#define __I_OPERATION_ANALYSIS_SCHEDULER_HPP

#include <memory>
#include <string>
#include <vector>
#include "primitives/definitions.hpp"
#include "primitives/user.hpp"
#include "primitives/user_configuration.hpp"
#include "primitives/analysis_task.hpp"

/**
 * @class IOperationAnalysisScheduler
 * @brief Abstract interface defining scheduler operations for asynchronous analysis tasks.
 */
class IOperationAnalysisScheduler
{
public:
    virtual ~IOperationAnalysisScheduler() = default;

    /**
     * @brief Starts the scheduler workers and initiates startup crash recovery.
     * @return rst_code_e RST_OK on success.
     */
    virtual rst_code_e start_scheduler() = 0;

    /**
     * @brief Gracefully stops scheduler workers and cancels or waits for active tasks.
     * @return rst_code_e RST_OK on success.
     */
    virtual rst_code_e stop_scheduler() = 0;

    /**
     * @brief Submits a new analysis task to the queue for a user's practice session.
     * @param user Logged-in user submitting the task.
     * @param practice_id Practice event ID to analyze.
     * @param config Configuration parameters struct.
     * @param out_task_id Output string receiving generated task UUID.
     * @return rst_code_e RST_OK on success, TASK_ALREADY_QUEUED if practice already active, or error code.
     */
    virtual rst_code_e submit_task(
        const std::shared_ptr<const User>& user,
        int practice_id,
        const UserConfiguration& config,
        std::string& out_task_id
    ) = 0;

    /**
     * @brief Cancels a waiting or running task belonging to the user.
     * @param user Logged-in user requesting cancellation.
     * @param task_id Unique GUID string of the task.
     * @return rst_code_e RST_OK on success, USER_NO_AUTH if user does not own task, or error code.
     */
    virtual rst_code_e cancel_task(
        const std::shared_ptr<const User>& user,
        const std::string& task_id
    ) = 0;

    /**
     * @brief Queries the status and progress of a specific task belonging to the user.
     * @param user Logged-in user requesting status.
     * @param task_id Unique GUID string of the task.
     * @param out_task Output parameter receiving task status.
     * @return rst_code_e RST_OK on success, USER_NO_AUTH if user does not own task, or TASK_NOT_FOUND.
     */
    virtual rst_code_e get_task_status(
        const std::shared_ptr<const User>& user,
        const std::string& task_id,
        AnalysisTask& out_task
    ) = 0;

    /**
     * @brief Retrieves all tasks submitted by the logged-in user.
     * @param user Logged-in user.
     * @param out_tasks Output list receiving tasks.
     * @return rst_code_e RST_OK on success, or error code.
     */
    virtual rst_code_e get_user_tasks(
        const std::shared_ptr<const User>& user,
        std::vector<AnalysisTask>& out_tasks
    ) = 0;

    /**
     * @brief Retrieves all tasks across all users (hidden/admin command).
     * @param admin_user Authenticated user executing the admin check.
     * @param out_tasks Output list receiving all tasks.
     * @return rst_code_e RST_OK on success, or error code.
     */
    virtual rst_code_e get_all_tasks(
        const std::shared_ptr<const User>& admin_user,
        std::vector<AnalysisTask>& out_tasks
    ) = 0;

    /**
     * @brief Configures maximum parallel tasks executed concurrently.
     * @param max_tasks Max parallel concurrency limit.
     */
    virtual void set_max_parallel_tasks(size_t max_tasks) = 0;

    /**
     * @brief Gets current maximum parallel tasks limit.
     * @return size_t Current concurrency limit.
     */
    virtual size_t get_max_parallel_tasks() const = 0;

    /**
     * @brief Gets the number of currently actively executing tasks.
     */
    virtual size_t get_running_tasks_count() const = 0;

    /**
     * @brief Gets the number of currently queued waiting tasks.
     */
    virtual size_t get_queued_tasks_count() const = 0;
};

#endif // __I_OPERATION_ANALYSIS_SCHEDULER_HPP
