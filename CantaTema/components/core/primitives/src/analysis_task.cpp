/**
 * @file analysis_task.cpp
 * @brief Implementation of the AnalysisTask domain model.
 */

#include "primitives/analysis_task.hpp"
#include <sstream>
#include <iomanip>
#include <chrono>

static std::time_t get_now_ms()
{
    return static_cast<std::time_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
}

AnalysisTask::AnalysisTask()
    : m_created_at(get_now_ms())
{
}

AnalysisTask::AnalysisTask(
    std::string task_id,
    unsigned int user_id,
    int practice_id,
    std::string config_snapshot_json
)
    : m_task_id(std::move(task_id)),
      m_user_id(user_id),
      m_practice_id(practice_id),
      m_status(AnalysisTaskStatus::QUEUED),
      m_progress_percentage(0),
      m_stage_description("Queued in scheduler"),
      m_config_snapshot_json(std::move(config_snapshot_json)),
      m_created_at(get_now_ms()),
      m_started_at(0),
      m_completed_at(0),
      m_retry_count(0)
{
}

const std::string& AnalysisTask::get_task_id() const
{
    return m_task_id;
}

unsigned int AnalysisTask::get_user_id() const
{
    return m_user_id;
}

int AnalysisTask::get_practice_id() const
{
    return m_practice_id;
}

AnalysisTaskStatus AnalysisTask::get_status() const
{
    return m_status;
}

int AnalysisTask::get_progress_percentage() const
{
    return m_progress_percentage;
}

const std::string& AnalysisTask::get_stage_description() const
{
    return m_stage_description;
}

const std::string& AnalysisTask::get_error_message() const
{
    return m_error_message;
}

rst_code_e AnalysisTask::get_result_code() const
{
    return m_result_code;
}

const std::string& AnalysisTask::get_execution_id() const
{
    return m_execution_id;
}

const std::string& AnalysisTask::get_config_snapshot_json() const
{
    return m_config_snapshot_json;
}

std::time_t AnalysisTask::get_created_at() const
{
    return m_created_at;
}

std::time_t AnalysisTask::get_started_at() const
{
    return m_started_at;
}

std::time_t AnalysisTask::get_completed_at() const
{
    return m_completed_at;
}

int AnalysisTask::get_retry_count() const
{
    return m_retry_count;
}

void AnalysisTask::set_task_id(std::string task_id)
{
    m_task_id = std::move(task_id);
}

void AnalysisTask::set_user_id(unsigned int user_id)
{
    m_user_id = user_id;
}

void AnalysisTask::set_practice_id(int practice_id)
{
    m_practice_id = practice_id;
}

void AnalysisTask::set_status(AnalysisTaskStatus status)
{
    m_status = status;
}

void AnalysisTask::set_progress_percentage(int progress)
{
    m_progress_percentage = progress;
}

void AnalysisTask::set_stage_description(std::string desc)
{
    m_stage_description = std::move(desc);
}

void AnalysisTask::set_error_message(std::string err)
{
    m_error_message = std::move(err);
}

void AnalysisTask::set_result_code(rst_code_e code)
{
    m_result_code = code;
}

void AnalysisTask::set_execution_id(std::string execution_id)
{
    m_execution_id = std::move(execution_id);
}

void AnalysisTask::set_config_snapshot_json(std::string config_json)
{
    m_config_snapshot_json = std::move(config_json);
}

void AnalysisTask::set_created_at(std::time_t created)
{
    m_created_at = created;
}

void AnalysisTask::set_started_at(std::time_t started)
{
    m_started_at = started;
}

void AnalysisTask::set_completed_at(std::time_t completed)
{
    m_completed_at = completed;
}

void AnalysisTask::set_retry_count(int count)
{
    m_retry_count = count;
}

std::string AnalysisTask::status_to_string(AnalysisTaskStatus status)
{
    switch (status)
    {
    case AnalysisTaskStatus::QUEUED:
        return "QUEUED";
    case AnalysisTaskStatus::CONVERTING_AUDIO:
        return "CONVERTING_AUDIO";
    case AnalysisTaskStatus::TRANSCRIBING:
        return "TRANSCRIBING";
    case AnalysisTaskStatus::GENERATING_EMBEDDINGS:
        return "GENERATING_EMBEDDINGS";
    case AnalysisTaskStatus::MATCHING_SIMILARITY:
        return "MATCHING_SIMILARITY";
    case AnalysisTaskStatus::COMPLETED:
        return "COMPLETED";
    case AnalysisTaskStatus::FAILED:
        return "FAILED";
    case AnalysisTaskStatus::CANCELLED:
        return "CANCELLED";
    default:
        return "UNKNOWN";
    }
}

AnalysisTaskStatus AnalysisTask::string_to_status(const std::string& status_str)
{
    if (status_str == "QUEUED") return AnalysisTaskStatus::QUEUED;
    if (status_str == "CONVERTING_AUDIO") return AnalysisTaskStatus::CONVERTING_AUDIO;
    if (status_str == "TRANSCRIBING") return AnalysisTaskStatus::TRANSCRIBING;
    if (status_str == "GENERATING_EMBEDDINGS") return AnalysisTaskStatus::GENERATING_EMBEDDINGS;
    if (status_str == "MATCHING_SIMILARITY") return AnalysisTaskStatus::MATCHING_SIMILARITY;
    if (status_str == "COMPLETED") return AnalysisTaskStatus::COMPLETED;
    if (status_str == "FAILED") return AnalysisTaskStatus::FAILED;
    if (status_str == "CANCELLED") return AnalysisTaskStatus::CANCELLED;
    return AnalysisTaskStatus::QUEUED;
}

bool AnalysisTask::is_finished() const
{
    return m_status == AnalysisTaskStatus::COMPLETED ||
           m_status == AnalysisTaskStatus::FAILED ||
           m_status == AnalysisTaskStatus::CANCELLED;
}

bool AnalysisTask::is_running() const
{
    return m_status == AnalysisTaskStatus::CONVERTING_AUDIO ||
           m_status == AnalysisTaskStatus::TRANSCRIBING ||
           m_status == AnalysisTaskStatus::GENERATING_EMBEDDINGS ||
           m_status == AnalysisTaskStatus::MATCHING_SIMILARITY;
}

std::string AnalysisTask::to_json() const
{
    std::ostringstream ss;
    ss << "{"
       << "\"task_id\":\"" << m_task_id << "\","
       << "\"user_id\":" << m_user_id << ","
       << "\"practice_id\":" << m_practice_id << ","
       << "\"status\":\"" << status_to_string(m_status) << "\","
       << "\"progress_percentage\":" << m_progress_percentage << ","
       << "\"stage_description\":\"" << m_stage_description << "\","
       << "\"error_message\":\"" << m_error_message << "\","
       << "\"result_code\":" << static_cast<int>(m_result_code) << ","
       << "\"execution_id\":\"" << m_execution_id << "\","
       << "\"created_at\":" << m_created_at << ","
       << "\"started_at\":" << m_started_at << ","
       << "\"completed_at\":" << m_completed_at << ","
       << "\"retry_count\":" << m_retry_count
       << "}";
    return ss.str();
}
