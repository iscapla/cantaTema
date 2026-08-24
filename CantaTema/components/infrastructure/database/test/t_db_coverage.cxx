#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <memory>
#include <vector>
#include <string>

// Mocks
#include "mock_tool_paths.hpp"

// Components under test
#include "database/db_coverage.hpp"
#include "database/db_practice_event.hpp"
#include "database/db_subject.hpp"
#include "database/db_user.hpp"
#include "database/db_connection.hpp"
#include "primitives/subject.hpp"
#include "primitives/user.hpp"
#include "primitives/practice_event.hpp"

using ::testing::Return;
using ::testing::_;

class DBCoverageTest : public ::testing::Test {
protected:
    MockToolPath mockToolPath;
    std::filesystem::path temp_db_dir;

    void SetUp() override {
        g_mockToolPath = &mockToolPath;

        temp_db_dir = std::filesystem::temp_directory_path() / ("canta_tema_test_db_cov_" + std::to_string(std::time(nullptr)));
        std::filesystem::create_directories(temp_db_dir);

        EXPECT_CALL(mockToolPath, get_path_for_database())
            .WillRepeatedly(Return(temp_db_dir));

        DB_Connection::reset_connection();

        DB_User db_user;
        ASSERT_EQ(db_user.user_tables_create(), RST_OK);

        DB_Subject db_subject;
        ASSERT_EQ(db_subject.subject_tables_create(), RST_OK);

        DB_PracticeEvent db_pe;
        ASSERT_EQ(db_pe.practice_event_tables_create(), RST_OK);

        DB_Coverage db_cov;
        ASSERT_EQ(db_cov.create_coverage_tables(), RST_OK);
    }

    void TearDown() override {
        DB_Connection::reset_connection();
        if (std::filesystem::exists(temp_db_dir)) {
            std::filesystem::remove_all(temp_db_dir);
        }
        g_mockToolPath = nullptr;
    }

    unsigned int create_test_user(const std::string& username) {
        DB_User db_user;
        User user(username);
        user.set_passwordkey("hashed_secret");
        user.set_roleid(1);
        user.set_creationdate(1234567890);
        user.set_status(User::Account_status::ACTIVE);
        
        if (db_user.add_new_user(user) == RST_OK) {
            return user.get_useraccountid();
        }
        return 0;
    }

    unsigned int create_test_subject(unsigned int user_id, const std::string& name) {
        DB_Subject db_subject;
        Subject subject(0, name);
        subject.set_user_id(user_id);
        
        if (db_subject.add_new_subject(subject) == RST_OK) {
            return subject.get_id();
        }
        return 0;
    }

    unsigned int create_test_practice_event(unsigned int user_id, unsigned int subject_id) {
        DB_PracticeEvent db_pe;
        PracticeEvent event;
        event.set_status(PracticeEvent::RECORDED);
        event.set_user_id(user_id);
        event.set_subject_id(subject_id);
        event.set_date(100000);
        event.set_recorded_date(100050);
        event.set_duration(120);
        event.set_description("Practice Session");
        event.set_filepath("/path/to/practice.opus");

        if (db_pe.add_new_practice_event(event) == RST_OK) {
            return event.get_id();
        }
        return 0;
    }
};

TEST_F(DBCoverageTest, SaveAndRetrieveCoverageExecution) {
    unsigned int user_id = create_test_user("cov_user_1");
    unsigned int subject_id = create_test_subject(user_id, "cov_subject_1");
    unsigned int practice_id = create_test_practice_event(user_id, subject_id);

    DB_Coverage db_cov;
    
    std::string exec_id = "exec_uuid_123456";
    std::string config_json = "{\"similarity_threshold\": 0.75}";
    std::string report_json = "{\"overall_coverage\": 85.5}";

    rst_code_e result = db_cov.save_coverage_analysis_execution(
        practice_id, exec_id, 85.5, 90.0, 95.0, 5.0,
        "small", "multilingual-e5-large", "es", 0.75,
        config_json, report_json
    );

    EXPECT_EQ(result, RST_OK);

    // Get executions list
    std::string json_list;
    rst_code_e list_res = db_cov.get_analysis_executions_for_practice(practice_id, json_list);
    EXPECT_EQ(list_res, RST_OK);
    EXPECT_NE(json_list.find(exec_id), std::string::npos);

    // Get details
    std::string details_report;
    std::string details_config;
    rst_code_e details_res = db_cov.get_analysis_execution_details(exec_id, details_report, details_config);
    EXPECT_EQ(details_res, RST_OK);
    EXPECT_EQ(details_report, report_json);
    EXPECT_EQ(details_config, config_json);
}

TEST_F(DBCoverageTest, DetailsNotFound) {
    DB_Coverage db_cov;
    std::string details_report;
    std::string details_config;
    rst_code_e details_res = db_cov.get_analysis_execution_details("non_existent_exec", details_report, details_config);
    EXPECT_EQ(details_res, DB_NOT_FOUND);
}

TEST_F(DBCoverageTest, SaveAndRetrieveAnalysisTask) {
    unsigned int user_id = create_test_user("task_user_1");
    unsigned int subject_id = create_test_subject(user_id, "task_subject_1");
    unsigned int practice_id = create_test_practice_event(user_id, subject_id);

    DB_Coverage db_cov;
    AnalysisTask task("task-uuid-1", user_id, static_cast<int>(practice_id), "{\"model\":\"test\"}");
    task.set_status(AnalysisTaskStatus::QUEUED);
    task.set_stage_description("Queued");

    EXPECT_EQ(db_cov.save_analysis_task(task), RST_OK);

    AnalysisTask retrieved;
    EXPECT_EQ(db_cov.get_analysis_task_by_id("task-uuid-1", retrieved), RST_OK);
    EXPECT_EQ(retrieved.get_task_id(), "task-uuid-1");
    EXPECT_EQ(retrieved.get_user_id(), user_id);
    EXPECT_EQ(retrieved.get_practice_id(), static_cast<int>(practice_id));
    EXPECT_EQ(retrieved.get_status(), AnalysisTaskStatus::QUEUED);
    EXPECT_EQ(retrieved.get_config_snapshot_json(), "{\"model\":\"test\"}");

    // Active task check
    AnalysisTask active;
    EXPECT_EQ(db_cov.get_active_analysis_task_for_practice(static_cast<int>(practice_id), active), RST_OK);
    EXPECT_EQ(active.get_task_id(), "task-uuid-1");

    // Update
    task.set_status(AnalysisTaskStatus::CONVERTING_AUDIO);
    task.set_progress_percentage(25);
    task.set_stage_description("Converting Opus to WAV");
    EXPECT_EQ(db_cov.update_analysis_task(task), RST_OK);

    EXPECT_EQ(db_cov.get_analysis_task_by_id("task-uuid-1", retrieved), RST_OK);
    EXPECT_EQ(retrieved.get_status(), AnalysisTaskStatus::CONVERTING_AUDIO);
    EXPECT_EQ(retrieved.get_progress_percentage(), 25);
    EXPECT_EQ(retrieved.get_stage_description(), "Converting Opus to WAV");

    // User and all queries
    std::vector<AnalysisTask> user_tasks;
    EXPECT_EQ(db_cov.get_analysis_tasks_by_user(user_id, user_tasks), RST_OK);
    EXPECT_EQ(user_tasks.size(), 1u);

    std::vector<AnalysisTask> all_tasks;
    EXPECT_EQ(db_cov.get_all_analysis_tasks(all_tasks), RST_OK);
    EXPECT_GE(all_tasks.size(), 1u);

    // Delete
    EXPECT_EQ(db_cov.delete_analysis_task("task-uuid-1"), RST_OK);
    EXPECT_EQ(db_cov.get_analysis_task_by_id("task-uuid-1", retrieved), TASK_NOT_FOUND);
}

TEST_F(DBCoverageTest, RecoverInterruptedAnalysisTasks) {
    unsigned int user_id = create_test_user("task_user_crash");
    unsigned int subject_id = create_test_subject(user_id, "task_subject_crash");
    unsigned int practice_id = create_test_practice_event(user_id, subject_id);

    DB_Coverage db_cov;

    // Task 1: Interrupted running task, retry_count 0 -> should recover to QUEUED with retry_count 1
    AnalysisTask task1("task-crash-1", user_id, static_cast<int>(practice_id));
    task1.set_status(AnalysisTaskStatus::TRANSCRIBING);
    task1.set_retry_count(0);
    EXPECT_EQ(db_cov.save_analysis_task(task1), RST_OK);

    // Task 2: Interrupted running task, retry_count 1 with max_retries 1 -> should fail
    AnalysisTask task2("task-crash-2", user_id, static_cast<int>(practice_id));
    task2.set_status(AnalysisTaskStatus::GENERATING_EMBEDDINGS);
    task2.set_retry_count(1);
    EXPECT_EQ(db_cov.save_analysis_task(task2), RST_OK);

    std::vector<AnalysisTask> recovered;
    EXPECT_EQ(db_cov.recover_interrupted_analysis_tasks(1, recovered), RST_OK);
    EXPECT_EQ(recovered.size(), 2u);

    AnalysisTask t1, t2;
    EXPECT_EQ(db_cov.get_analysis_task_by_id("task-crash-1", t1), RST_OK);
    EXPECT_EQ(t1.get_status(), AnalysisTaskStatus::QUEUED);
    EXPECT_EQ(t1.get_retry_count(), 1);

    EXPECT_EQ(db_cov.get_analysis_task_by_id("task-crash-2", t2), RST_OK);
    EXPECT_EQ(t2.get_status(), AnalysisTaskStatus::FAILED);
}

TEST_F(DBCoverageTest, DBCloseDatabaseConnectionErrors) {
    DB_Coverage db_cov;
    sqlite3_close(DB_Connection::getConn().get());

    EXPECT_EQ(db_cov.create_coverage_tables(), DB_FAIL);
    EXPECT_EQ(db_cov.save_coverage_analysis_execution(1, "x", 0, 0, 0, 0, "", "", "", 0, "", ""), DB_FAIL);
    
    std::string tmp;
    EXPECT_EQ(db_cov.get_analysis_executions_for_practice(1, tmp), DB_FAIL);
    
    std::string tmp2;
    EXPECT_EQ(db_cov.get_analysis_execution_details("x", tmp, tmp2), DB_FAIL);

    AnalysisTask dummy_task;
    EXPECT_EQ(db_cov.create_analysis_task_tables(), DB_FAIL);
    EXPECT_EQ(db_cov.save_analysis_task(dummy_task), DB_FAIL);
    EXPECT_EQ(db_cov.update_analysis_task(dummy_task), DB_FAIL);
    EXPECT_EQ(db_cov.get_analysis_task_by_id("x", dummy_task), DB_FAIL);
    
    std::vector<AnalysisTask> tasks;
    EXPECT_EQ(db_cov.get_analysis_tasks_by_user(1, tasks), DB_FAIL);
    EXPECT_EQ(db_cov.get_all_analysis_tasks(tasks), DB_FAIL);
    EXPECT_EQ(db_cov.get_active_analysis_task_for_practice(1, dummy_task), DB_FAIL);
    EXPECT_EQ(db_cov.delete_analysis_task("x"), DB_FAIL);

    DB_Connection::reset_connection();
    db_cov.create_coverage_tables();
}
