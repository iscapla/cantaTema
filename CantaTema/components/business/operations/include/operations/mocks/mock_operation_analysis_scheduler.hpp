/**
 * @file mock_operation_analysis_scheduler.hpp
 * @brief Mock implementation of IOperationAnalysisScheduler for unit testing.
 */

#ifndef __MOCK_OPERATION_ANALYSIS_SCHEDULER_HPP
#define __MOCK_OPERATION_ANALYSIS_SCHEDULER_HPP

#include <gmock/gmock.h>
#include "operations/i_operation_analysis_scheduler.hpp"

class MockOperationAnalysisScheduler : public IOperationAnalysisScheduler
{
public:
    MOCK_METHOD(rst_code_e, start_scheduler, (), (override));
    MOCK_METHOD(rst_code_e, stop_scheduler, (), (override));

    MOCK_METHOD(rst_code_e, submit_task, (
        const std::shared_ptr<const User>& user,
        int practice_id,
        const UserConfiguration& config,
        std::string& out_task_id
    ), (override));

    MOCK_METHOD(rst_code_e, cancel_task, (
        const std::shared_ptr<const User>& user,
        const std::string& task_id
    ), (override));

    MOCK_METHOD(rst_code_e, get_task_status, (
        const std::shared_ptr<const User>& user,
        const std::string& task_id,
        AnalysisTask& out_task
    ), (override));

    MOCK_METHOD(rst_code_e, get_user_tasks, (
        const std::shared_ptr<const User>& user,
        std::vector<AnalysisTask>& out_tasks
    ), (override));

    MOCK_METHOD(rst_code_e, get_all_tasks, (
        const std::shared_ptr<const User>& admin_user,
        std::vector<AnalysisTask>& out_tasks
    ), (override));

    MOCK_METHOD(void, set_max_parallel_tasks, (size_t max_tasks), (override));
    MOCK_METHOD(size_t, get_max_parallel_tasks, (), (const, override));
    MOCK_METHOD(size_t, get_running_tasks_count, (), (const, override));
    MOCK_METHOD(size_t, get_queued_tasks_count, (), (const, override));
};

#endif // __MOCK_OPERATION_ANALYSIS_SCHEDULER_HPP
