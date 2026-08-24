#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>

#include "operations/operation_analysis_scheduler.hpp"
#include "database/mocks/mock_database.hpp"
#include "operations/mocks/mock_operation_coverage.hpp"
#include "operations/mocks/mock_operation_practice_event.hpp"
#include "operations/operation_user.hpp"
#include "database/db_coverage.hpp"
#include "database/db_main.hpp"
#include "primitives/user.hpp"
#include "primitives/practice_event.hpp"

using ::testing::_;
using ::testing::Return;
using ::testing::DoAll;
using ::testing::SetArgReferee;

class OperationAnalysisSchedulerTest : public ::testing::Test {
protected:
    std::shared_ptr<DB_Coverage> real_db;
    std::shared_ptr<User> user1;
    std::shared_ptr<User> user2;
    std::shared_ptr<User> admin_user;

    void SetUp() override {
        DB_Main::getInstance()->purge();
        real_db = std::make_shared<DB_Coverage>();
        real_db->create_coverage_tables();

        user1 = std::make_shared<User>("user_one");
        user1->set_useraccountid(101);
        user1->set_is_authenticated(true);
        user1->set_roleid(1);

        user2 = std::make_shared<User>("user_two");
        user2->set_useraccountid(102);
        user2->set_is_authenticated(true);
        user2->set_roleid(1);

        admin_user = std::make_shared<User>("admin_user");
        admin_user->set_useraccountid(999);
        admin_user->set_is_authenticated(true);
        admin_user->set_roleid(0); // Admin
    }

    void TearDown() override {
        DB_Main::getInstance()->purge();
    }
};

//-----------------------------------------------------------------------------------------
// 1. SPECIFIC TEST: Scheduler Concurrency Limit (Max 1 Task by default) with Dummy Tasks
//-----------------------------------------------------------------------------------------
TEST_F(OperationAnalysisSchedulerTest, SpecificTest_DummyTasksEnforceMaxOneParallelLimit) {
    std::atomic<int> active_concurrent_count{0};
    std::atomic<int> max_observed_concurrency{0};
    std::vector<std::string> completed_task_order;
    std::mutex order_mutex;

    // Create scheduler with dummy task executor
    auto scheduler = std::make_unique<OperationAnalysisScheduler>(
        real_db,
        nullptr,
        nullptr,
        nullptr,
        [&](const std::shared_ptr<const User>& u, AnalysisTask& task, std::shared_ptr<std::atomic<bool>> cancel_token, std::string& out_exec_id) -> rst_code_e {
            int current = ++active_concurrent_count;
            int prev_max = max_observed_concurrency.load();
            while (current > prev_max && !max_observed_concurrency.compare_exchange_weak(prev_max, current)) {
                prev_max = max_observed_concurrency.load();
            }

            // Simulate multi-stage processing with dummy delays
            for (int stage = 0; stage < 5; ++stage) {
                if (cancel_token && cancel_token->load()) {
                    --active_concurrent_count;
                    return TASK_CANCELLED;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }

            out_exec_id = "exec-" + task.get_task_id();
            {
                std::lock_guard<std::mutex> lock(order_mutex);
                completed_task_order.push_back(task.get_task_id());
            }
            --active_concurrent_count;
            return RST_OK;
        }
    );

    scheduler->set_max_parallel_tasks(1);
    EXPECT_EQ(scheduler->start_scheduler(), RST_OK);

    // Submit 3 dummy tasks from different users simultaneously
    std::string task1_id, task2_id, task3_id;
    UserConfiguration cfg;

    EXPECT_EQ(scheduler->submit_task(user1, 1, cfg, task1_id), RST_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    EXPECT_EQ(scheduler->submit_task(user2, 2, cfg, task2_id), RST_OK);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    EXPECT_EQ(scheduler->submit_task(user1, 3, cfg, task3_id), RST_OK);

    // Wait for all tasks to complete (max ~3 seconds timeout)
    for (int i = 0; i < 100; ++i) {
        AnalysisTask t1, t2, t3;
        scheduler->get_task_status(user1, task1_id, t1);
        scheduler->get_task_status(user2, task2_id, t2);
        scheduler->get_task_status(user1, task3_id, t3);

        if (t1.is_finished() && t2.is_finished() && t3.is_finished()) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }

    scheduler->stop_scheduler();

    // Verify maximum concurrency was exactly 1
    EXPECT_EQ(max_observed_concurrency.load(), 1);

    // Verify all tasks completed in FIFO order
    ASSERT_EQ(completed_task_order.size(), 3u);
    EXPECT_EQ(completed_task_order[0], task1_id);
    EXPECT_EQ(completed_task_order[1], task2_id);
    EXPECT_EQ(completed_task_order[2], task3_id);

    // Verify final task statuses
    AnalysisTask t1, t2, t3;
    EXPECT_EQ(scheduler->get_task_status(user1, task1_id, t1), RST_OK);
    EXPECT_EQ(t1.get_status(), AnalysisTaskStatus::COMPLETED);
    EXPECT_EQ(t1.get_progress_percentage(), 100);
    EXPECT_EQ(t1.get_execution_id(), "exec-" + task1_id);

    EXPECT_EQ(scheduler->get_task_status(user2, task2_id, t2), RST_OK);
    EXPECT_EQ(t2.get_status(), AnalysisTaskStatus::COMPLETED);

    EXPECT_EQ(scheduler->get_task_status(user1, task3_id, t3), RST_OK);
    EXPECT_EQ(t3.get_status(), AnalysisTaskStatus::COMPLETED);
}

//-----------------------------------------------------------------------------------------
// 2. Cancellation of Queued and Actively Running Tasks
//-----------------------------------------------------------------------------------------
TEST_F(OperationAnalysisSchedulerTest, TaskCancellationQueuedAndRunning) {
    std::atomic<bool> task1_started{false};

    auto scheduler = std::make_unique<OperationAnalysisScheduler>(
        real_db,
        nullptr,
        nullptr,
        nullptr,
        [&](const std::shared_ptr<const User>& u, AnalysisTask& task, std::shared_ptr<std::atomic<bool>> cancel_token, std::string& out_exec_id) -> rst_code_e {
            task1_started = true;
            for (int i = 0; i < 50; ++i) {
                if (cancel_token && cancel_token->load()) {
                    return TASK_CANCELLED;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            out_exec_id = "exec-" + task.get_task_id();
            return RST_OK;
        }
    );

    scheduler->set_max_parallel_tasks(1);
    EXPECT_EQ(scheduler->start_scheduler(), RST_OK);

    std::string task1_id, task2_id;
    UserConfiguration cfg;
    EXPECT_EQ(scheduler->submit_task(user1, 10, cfg, task1_id), RST_OK);
    EXPECT_EQ(scheduler->submit_task(user1, 20, cfg, task2_id), RST_OK);

    // Wait until task 1 starts
    for (int i = 0; i < 50 && !task1_started; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Cancel queued task 2 immediately
    EXPECT_EQ(scheduler->cancel_task(user1, task2_id), RST_OK);
    AnalysisTask t2_status;
    EXPECT_EQ(scheduler->get_task_status(user1, task2_id, t2_status), RST_OK);
    EXPECT_EQ(t2_status.get_status(), AnalysisTaskStatus::CANCELLED);

    // Cancel running task 1
    EXPECT_EQ(scheduler->cancel_task(user1, task1_id), RST_OK);

    // Wait for worker to finish
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    scheduler->stop_scheduler();

    AnalysisTask t1_status;
    EXPECT_EQ(scheduler->get_task_status(user1, task1_id, t1_status), RST_OK);
    EXPECT_EQ(t1_status.get_status(), AnalysisTaskStatus::CANCELLED);
}

//-----------------------------------------------------------------------------------------
// 3. Multi-Tenancy & User Access Control Isolation
//-----------------------------------------------------------------------------------------
TEST_F(OperationAnalysisSchedulerTest, MultiTenancyUserIsolation) {
    auto scheduler = std::make_unique<OperationAnalysisScheduler>(real_db);
    std::string task1_id;
    UserConfiguration cfg;

    EXPECT_EQ(scheduler->submit_task(user1, 55, cfg, task1_id), RST_OK);

    // User 2 cannot access User 1's task status
    AnalysisTask query_task;
    EXPECT_EQ(scheduler->get_task_status(user2, task1_id, query_task), USER_NO_AUTH);

    // User 2 cannot cancel User 1's task
    EXPECT_EQ(scheduler->cancel_task(user2, task1_id), USER_NO_AUTH);

    // User 1 CAN query and cancel their task
    EXPECT_EQ(scheduler->get_task_status(user1, task1_id, query_task), RST_OK);
    EXPECT_EQ(query_task.get_task_id(), task1_id);

    // User query returns only their tasks
    std::vector<AnalysisTask> u1_tasks, u2_tasks;
    EXPECT_EQ(scheduler->get_user_tasks(user1, u1_tasks), RST_OK);
    EXPECT_EQ(u1_tasks.size(), 1u);

    EXPECT_EQ(scheduler->get_user_tasks(user2, u2_tasks), RST_OK);
    EXPECT_EQ(u2_tasks.size(), 0u);

    // Admin can query all tasks
    std::vector<AnalysisTask> all_tasks;
    EXPECT_EQ(scheduler->get_all_tasks(admin_user, all_tasks), RST_OK);
    EXPECT_EQ(all_tasks.size(), 1u);
}

//-----------------------------------------------------------------------------------------
// 4. Duplicate Task Guard
//-----------------------------------------------------------------------------------------
TEST_F(OperationAnalysisSchedulerTest, DuplicateTaskGuardForSamePractice) {
    auto scheduler = std::make_unique<OperationAnalysisScheduler>(real_db);
    std::string task1_id, task2_id;
    UserConfiguration cfg;

    EXPECT_EQ(scheduler->submit_task(user1, 77, cfg, task1_id), RST_OK);

    // Submitting again for the same practice ID 77 returns TASK_ALREADY_QUEUED
    EXPECT_EQ(scheduler->submit_task(user1, 77, cfg, task2_id), TASK_ALREADY_QUEUED);
    EXPECT_EQ(task1_id, task2_id);
}

//-----------------------------------------------------------------------------------------
// 5. Crash Recovery & Max Retries Enforcement
//-----------------------------------------------------------------------------------------
TEST_F(OperationAnalysisSchedulerTest, CrashRecoveryResumesTasksUpToMaxRetries) {
    // Manually create interrupted task in DB
    AnalysisTask interrupted_task("task-interrupted", user1->get_useraccountid(), 88);
    interrupted_task.set_status(AnalysisTaskStatus::TRANSCRIBING);
    interrupted_task.set_retry_count(0);
    real_db->save_analysis_task(interrupted_task);

    std::atomic<bool> recovered_task_executed{false};
    auto scheduler = std::make_unique<OperationAnalysisScheduler>(
        real_db,
        nullptr,
        nullptr,
        nullptr,
        [&](const std::shared_ptr<const User>& u, AnalysisTask& task, std::shared_ptr<std::atomic<bool>> cancel_token, std::string& out_exec_id) -> rst_code_e {
            if (task.get_task_id() == "task-interrupted") {
                recovered_task_executed = true;
            }
            out_exec_id = "exec-recovered";
            return RST_OK;
        }
    );

    EXPECT_EQ(scheduler->start_scheduler(), RST_OK);

    for (int i = 0; i < 50 && !recovered_task_executed; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    scheduler->stop_scheduler();

    EXPECT_TRUE(recovered_task_executed.load());
    AnalysisTask check_task;
    EXPECT_EQ(scheduler->get_task_status(user1, "task-interrupted", check_task), RST_OK);
    EXPECT_EQ(check_task.get_status(), AnalysisTaskStatus::COMPLETED);
    EXPECT_EQ(check_task.get_retry_count(), 1);
}

//-----------------------------------------------------------------------------------------
// 6. Error and Boundary Cases
//-----------------------------------------------------------------------------------------
TEST_F(OperationAnalysisSchedulerTest, ErrorAndBoundaryCases) {
    auto scheduler = std::make_unique<OperationAnalysisScheduler>(real_db);
    std::string task_id;
    UserConfiguration cfg;

    // Null user
    EXPECT_EQ(scheduler->submit_task(nullptr, 1, cfg, task_id), USER_NO_AUTH);
    EXPECT_EQ(scheduler->cancel_task(nullptr, "x"), USER_NO_AUTH);

    AnalysisTask t;
    EXPECT_EQ(scheduler->get_task_status(nullptr, "x", t), USER_NO_AUTH);

    std::vector<AnalysisTask> list;
    EXPECT_EQ(scheduler->get_user_tasks(nullptr, list), USER_NO_AUTH);
    EXPECT_EQ(scheduler->get_all_tasks(nullptr, list), USER_NO_AUTH);

    // Non-existent task
    EXPECT_EQ(scheduler->get_task_status(user1, "non_existent", t), TASK_NOT_FOUND);
    EXPECT_EQ(scheduler->cancel_task(user1, "non_existent"), TASK_NOT_FOUND);

    // Null database error
    OperationAnalysisScheduler null_scheduler(nullptr);
    EXPECT_EQ(null_scheduler.start_scheduler(), UNKNOWN);
    EXPECT_EQ(null_scheduler.submit_task(user1, 1, cfg, task_id), UNKNOWN);
}
