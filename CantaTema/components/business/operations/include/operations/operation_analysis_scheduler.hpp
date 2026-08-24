/**
 * @file operation_analysis_scheduler.hpp
 * @brief Concrete implementation of the asynchronous analysis task scheduler.
 */

#ifndef __OPERATION_ANALYSIS_SCHEDULER_HPP
#define __OPERATION_ANALYSIS_SCHEDULER_HPP

#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <unordered_map>
#include <unordered_set>
#include <functional>

#include "operations/i_operation_analysis_scheduler.hpp"
#include "operations/i_operation_coverage.hpp"
#include "operations/i_operation_practice_event.hpp"
#include "operations/i_operation_user.hpp"
#include "database/i_database.hpp"

/**
 * @class OperationAnalysisScheduler
 * @brief Coordinates multi-user task queuing, single/configurable concurrency execution,
 * task cancellation, startup recovery, and user task access control.
 */
class OperationAnalysisScheduler : public IOperationAnalysisScheduler
{
public:
    /**
     * @brief Custom executor callback signature for executing analysis (allows injecting dummy test tasks).
     */
    using TaskExecutorFn = std::function<rst_code_e(
        const std::shared_ptr<const User>& user,
        AnalysisTask& task,
        std::shared_ptr<std::atomic<bool>> cancel_token,
        std::string& out_execution_id
    )>;

    /**
     * @brief Constructs OperationAnalysisScheduler with injected dependencies.
     * @param db Injected database repository.
     * @param coverage_op Injected coverage operations pipeline.
     * @param practice_op Injected practice event operations.
     * @param user_op Injected user operations.
     * @param custom_executor Optional custom execution function (for tests with dummy tasks).
     */
    OperationAnalysisScheduler(
        std::shared_ptr<IDatabase> db = nullptr,
        std::shared_ptr<IOperationCoverage> coverage_op = nullptr,
        std::shared_ptr<IOperationPracticeEvent> practice_op = nullptr,
        std::shared_ptr<IOperationUser> user_op = nullptr,
        TaskExecutorFn custom_executor = nullptr
    );

    ~OperationAnalysisScheduler() override;

    rst_code_e start_scheduler() override;
    rst_code_e stop_scheduler() override;

    rst_code_e submit_task(
        const std::shared_ptr<const User>& user,
        int practice_id,
        const UserConfiguration& config,
        std::string& out_task_id
    ) override;

    rst_code_e cancel_task(
        const std::shared_ptr<const User>& user,
        const std::string& task_id
    ) override;

    rst_code_e get_task_status(
        const std::shared_ptr<const User>& user,
        const std::string& task_id,
        AnalysisTask& out_task
    ) override;

    rst_code_e get_user_tasks(
        const std::shared_ptr<const User>& user,
        std::vector<AnalysisTask>& out_tasks
    ) override;

    rst_code_e get_all_tasks(
        const std::shared_ptr<const User>& admin_user,
        std::vector<AnalysisTask>& out_tasks
    ) override;

    void set_max_parallel_tasks(size_t max_tasks) override;
    size_t get_max_parallel_tasks() const override;
    size_t get_running_tasks_count() const override;
    size_t get_queued_tasks_count() const override;

    /**
     * @brief Sets custom executor function for testing dummy tasks.
     */
    void set_custom_executor(TaskExecutorFn executor);

private:
    std::shared_ptr<IDatabase> m_db;
    std::shared_ptr<IOperationCoverage> m_coverage_op;
    std::shared_ptr<IOperationPracticeEvent> m_practice_op;
    std::shared_ptr<IOperationUser> m_user_op;
    TaskExecutorFn m_custom_executor;

    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_running{false};
    size_t m_max_parallel_tasks{1};

    std::vector<std::thread> m_worker_threads;
    std::unordered_set<std::string> m_running_task_ids;
    std::unordered_map<std::string, std::shared_ptr<std::atomic<bool>>> m_active_cancellation_tokens;

    std::string generate_task_uuid() const;
    void worker_loop();
    void execute_single_task(AnalysisTask task, std::shared_ptr<std::atomic<bool>> cancel_token);
    rst_code_e default_execute_coverage(
        const std::shared_ptr<const User>& user,
        AnalysisTask& task,
        std::shared_ptr<std::atomic<bool>> cancel_token,
        std::string& out_execution_id
    );
};

#endif // __OPERATION_ANALYSIS_SCHEDULER_HPP
