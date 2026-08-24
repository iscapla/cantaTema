#include <gtest/gtest.h>
#include "primitives/analysis_task.hpp"

TEST(AnalysisTaskTest, DefaultConstructorInitializesProperties)
{
    AnalysisTask task;
    EXPECT_TRUE(task.get_task_id().empty());
    EXPECT_EQ(task.get_user_id(), 0u);
    EXPECT_EQ(task.get_practice_id(), 0);
    EXPECT_EQ(task.get_status(), AnalysisTaskStatus::QUEUED);
    EXPECT_EQ(task.get_progress_percentage(), 0);
    EXPECT_FALSE(task.is_finished());
    EXPECT_FALSE(task.is_running());
    EXPECT_EQ(task.get_retry_count(), 0);
}

TEST(AnalysisTaskTest, ParameterizedConstructorAndGettersSetters)
{
    AnalysisTask task("task-uuid-123", 42, 101, "{\"key\":\"value\"}");
    EXPECT_EQ(task.get_task_id(), "task-uuid-123");
    EXPECT_EQ(task.get_user_id(), 42u);
    EXPECT_EQ(task.get_practice_id(), 101);
    EXPECT_EQ(task.get_config_snapshot_json(), "{\"key\":\"value\"}");
    EXPECT_EQ(task.get_status(), AnalysisTaskStatus::QUEUED);

    task.set_task_id("new-id");
    EXPECT_EQ(task.get_task_id(), "new-id");

    task.set_user_id(99);
    EXPECT_EQ(task.get_user_id(), 99u);

    task.set_practice_id(202);
    EXPECT_EQ(task.get_practice_id(), 202);

    task.set_status(AnalysisTaskStatus::TRANSCRIBING);
    EXPECT_EQ(task.get_status(), AnalysisTaskStatus::TRANSCRIBING);
    EXPECT_TRUE(task.is_running());
    EXPECT_FALSE(task.is_finished());

    task.set_progress_percentage(50);
    EXPECT_EQ(task.get_progress_percentage(), 50);

    task.set_stage_description("Transcribing audio");
    EXPECT_EQ(task.get_stage_description(), "Transcribing audio");

    task.set_error_message("none");
    EXPECT_EQ(task.get_error_message(), "none");

    task.set_result_code(RST_OK);
    EXPECT_EQ(task.get_result_code(), RST_OK);

    task.set_execution_id("exec-guid-456");
    EXPECT_EQ(task.get_execution_id(), "exec-guid-456");

    task.set_config_snapshot_json("{}");
    EXPECT_EQ(task.get_config_snapshot_json(), "{}");

    std::time_t now = std::time(nullptr);
    task.set_created_at(now);
    task.set_started_at(now + 1);
    task.set_completed_at(now + 10);
    EXPECT_EQ(task.get_created_at(), now);
    EXPECT_EQ(task.get_started_at(), now + 1);
    EXPECT_EQ(task.get_completed_at(), now + 10);

    task.set_retry_count(2);
    EXPECT_EQ(task.get_retry_count(), 2);
}

TEST(AnalysisTaskTest, StatusConversionsAndTransitions)
{
    EXPECT_EQ(AnalysisTask::status_to_string(AnalysisTaskStatus::QUEUED), "QUEUED");
    EXPECT_EQ(AnalysisTask::status_to_string(AnalysisTaskStatus::CONVERTING_AUDIO), "CONVERTING_AUDIO");
    EXPECT_EQ(AnalysisTask::status_to_string(AnalysisTaskStatus::TRANSCRIBING), "TRANSCRIBING");
    EXPECT_EQ(AnalysisTask::status_to_string(AnalysisTaskStatus::GENERATING_EMBEDDINGS), "GENERATING_EMBEDDINGS");
    EXPECT_EQ(AnalysisTask::status_to_string(AnalysisTaskStatus::MATCHING_SIMILARITY), "MATCHING_SIMILARITY");
    EXPECT_EQ(AnalysisTask::status_to_string(AnalysisTaskStatus::COMPLETED), "COMPLETED");
    EXPECT_EQ(AnalysisTask::status_to_string(AnalysisTaskStatus::FAILED), "FAILED");
    EXPECT_EQ(AnalysisTask::status_to_string(AnalysisTaskStatus::CANCELLED), "CANCELLED");
    EXPECT_EQ(AnalysisTask::status_to_string(static_cast<AnalysisTaskStatus>(999)), "UNKNOWN");

    EXPECT_EQ(AnalysisTask::string_to_status("QUEUED"), AnalysisTaskStatus::QUEUED);
    EXPECT_EQ(AnalysisTask::string_to_status("CONVERTING_AUDIO"), AnalysisTaskStatus::CONVERTING_AUDIO);
    EXPECT_EQ(AnalysisTask::string_to_status("TRANSCRIBING"), AnalysisTaskStatus::TRANSCRIBING);
    EXPECT_EQ(AnalysisTask::string_to_status("GENERATING_EMBEDDINGS"), AnalysisTaskStatus::GENERATING_EMBEDDINGS);
    EXPECT_EQ(AnalysisTask::string_to_status("MATCHING_SIMILARITY"), AnalysisTaskStatus::MATCHING_SIMILARITY);
    EXPECT_EQ(AnalysisTask::string_to_status("COMPLETED"), AnalysisTaskStatus::COMPLETED);
    EXPECT_EQ(AnalysisTask::string_to_status("FAILED"), AnalysisTaskStatus::FAILED);
    EXPECT_EQ(AnalysisTask::string_to_status("CANCELLED"), AnalysisTaskStatus::CANCELLED);
    EXPECT_EQ(AnalysisTask::string_to_status("INVALID_STR"), AnalysisTaskStatus::QUEUED);
}

TEST(AnalysisTaskTest, IsRunningAndIsFinishedStates)
{
    AnalysisTask task;
    task.set_status(AnalysisTaskStatus::QUEUED);
    EXPECT_FALSE(task.is_running());
    EXPECT_FALSE(task.is_finished());

    task.set_status(AnalysisTaskStatus::CONVERTING_AUDIO);
    EXPECT_TRUE(task.is_running());
    EXPECT_FALSE(task.is_finished());

    task.set_status(AnalysisTaskStatus::GENERATING_EMBEDDINGS);
    EXPECT_TRUE(task.is_running());
    EXPECT_FALSE(task.is_finished());

    task.set_status(AnalysisTaskStatus::MATCHING_SIMILARITY);
    EXPECT_TRUE(task.is_running());
    EXPECT_FALSE(task.is_finished());

    task.set_status(AnalysisTaskStatus::COMPLETED);
    EXPECT_FALSE(task.is_running());
    EXPECT_TRUE(task.is_finished());

    task.set_status(AnalysisTaskStatus::FAILED);
    EXPECT_FALSE(task.is_running());
    EXPECT_TRUE(task.is_finished());

    task.set_status(AnalysisTaskStatus::CANCELLED);
    EXPECT_FALSE(task.is_running());
    EXPECT_TRUE(task.is_finished());
}

TEST(AnalysisTaskTest, ToJsonSerialization)
{
    AnalysisTask task("task-abc", 1, 5);
    task.set_status(AnalysisTaskStatus::COMPLETED);
    task.set_progress_percentage(100);
    task.set_stage_description("Done");
    task.set_execution_id("exec-xyz");

    std::string json = task.to_json();
    EXPECT_NE(json.find("\"task_id\":\"task-abc\""), std::string::npos);
    EXPECT_NE(json.find("\"user_id\":1"), std::string::npos);
    EXPECT_NE(json.find("\"practice_id\":5"), std::string::npos);
    EXPECT_NE(json.find("\"status\":\"COMPLETED\""), std::string::npos);
    EXPECT_NE(json.find("\"progress_percentage\":100"), std::string::npos);
    EXPECT_NE(json.find("\"stage_description\":\"Done\""), std::string::npos);
    EXPECT_NE(json.find("\"execution_id\":\"exec-xyz\""), std::string::npos);
}
