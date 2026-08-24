/**
 * @file operation_analysis_scheduler.cpp
 * @brief Implementation of OperationAnalysisScheduler.
 */

#include "operations/operation_analysis_scheduler.hpp"
#include "configuration/configuration_system.hpp"
#include "primitives/utils_logger.hpp"

#include <chrono>
#include <random>
#include <sstream>

OperationAnalysisScheduler::OperationAnalysisScheduler(
    std::shared_ptr<IDatabase> db,
    std::shared_ptr<IOperationCoverage> coverage_op,
    std::shared_ptr<IOperationPracticeEvent> practice_op,
    std::shared_ptr<IOperationUser> user_op,
    TaskExecutorFn custom_executor
)
    : m_db(std::move(db)),
      m_coverage_op(std::move(coverage_op)),
      m_practice_op(std::move(practice_op)),
      m_user_op(std::move(user_op)),
      m_custom_executor(std::move(custom_executor))
{
    m_max_parallel_tasks = ConfigurationSystem::getInstance().get_scheduler_max_parallel_tasks();
    if (m_max_parallel_tasks == 0) {
        m_max_parallel_tasks = 1;
    }
}

OperationAnalysisScheduler::~OperationAnalysisScheduler()
{
    stop_scheduler();
}

std::string OperationAnalysisScheduler::generate_task_uuid() const
{
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint32_t> dis(100000, 999999);

    std::ostringstream ss;
    ss << "task-" << timestamp << "-" << dis(gen);
    return ss.str();
}

rst_code_e OperationAnalysisScheduler::start_scheduler()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_running) {
        return RST_OK;
    }

    if (!m_db) {
        if (logger) logger->error("OperationAnalysisScheduler::start_scheduler - Missing database dependency");
        return UNKNOWN;
    }

    m_running = true;

    // Crash recovery
    if (ConfigurationSystem::getInstance().get_scheduler_auto_resume_on_startup()) {
        int max_retries = static_cast<int>(ConfigurationSystem::getInstance().get_scheduler_max_retries());
        std::vector<AnalysisTask> recovered;
        m_db->recover_interrupted_analysis_tasks(max_retries, recovered);
        if (!recovered.empty() && logger) {
            logger->info("OperationAnalysisScheduler - Crash recovery recovered {} interrupted tasks", recovered.size());
        }
    }

    // Launch worker threads
    m_worker_threads.clear();
    for (size_t i = 0; i < m_max_parallel_tasks; ++i) {
        m_worker_threads.emplace_back(&OperationAnalysisScheduler::worker_loop, this);
    }

    if (logger) logger->info("OperationAnalysisScheduler started with {} worker thread(s)", m_max_parallel_tasks);
    return RST_OK;
}

rst_code_e OperationAnalysisScheduler::stop_scheduler()
{
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (!m_running) {
            return RST_OK;
        }
        m_running = false;

        // Signal all active cancellation tokens
        for (auto& [id, token] : m_active_cancellation_tokens) {
            if (token) {
                token->store(true);
            }
        }
    }

    m_cv.notify_all();

    for (auto& worker : m_worker_threads) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    m_worker_threads.clear();

    if (logger) logger->info("OperationAnalysisScheduler stopped");
    return RST_OK;
}

rst_code_e OperationAnalysisScheduler::submit_task(
    const std::shared_ptr<const User>& user,
    int practice_id,
    const UserConfiguration& config,
    std::string& out_task_id
)
{
    if (!user) {
        if (logger) logger->error("OperationAnalysisScheduler::submit_task - User parameter is null");
        return USER_NO_AUTH;
    }

    if (!m_db) {
        if (logger) logger->error("OperationAnalysisScheduler::submit_task - Missing database repository");
        return UNKNOWN;
    }

    // Duplicate check
    AnalysisTask active_task;
    rst_code_e chk = m_db->get_active_analysis_task_for_practice(practice_id, active_task);
    if (chk == RST_OK && !active_task.is_finished()) {
        out_task_id = active_task.get_task_id();
        if (logger) logger->warn("OperationAnalysisScheduler::submit_task - Practice ID {} already has active task {}", practice_id, out_task_id);
        return TASK_ALREADY_QUEUED;
    }

    // Verify practice exists if practice operation injected
    if (m_practice_op) {
        std::shared_ptr<PracticeEvent> pe;
        rst_code_e res = m_practice_op->practice_event_get_by_id(user, static_cast<unsigned int>(practice_id), pe);
        if (res != RST_OK || !pe) {
            if (logger) logger->error("OperationAnalysisScheduler::submit_task - Practice event {} not found", practice_id);
            return (res != RST_OK) ? res : PRACTICE_EVENT_NOT_FOUND;
        }
    }

    std::string task_id = generate_task_uuid();
    std::string config_json = config.to_json();

    AnalysisTask task(task_id, user->get_useraccountid(), practice_id, config_json);
    task.set_status(AnalysisTaskStatus::QUEUED);
    task.set_stage_description("Queued in scheduler");
    task.set_progress_percentage(0);

    rst_code_e save_res = m_db->save_analysis_task(task);
    if (save_res != RST_OK) {
        if (logger) logger->error("OperationAnalysisScheduler::submit_task - Failed to persist task {}", task_id);
        return save_res;
    }

    out_task_id = task_id;
    if (logger) logger->info("OperationAnalysisScheduler - Task {} queued for practice {} by user {}", task_id, practice_id, user->get_useraccountid());

    m_cv.notify_one();
    return RST_OK;
}

rst_code_e OperationAnalysisScheduler::cancel_task(
    const std::shared_ptr<const User>& user,
    const std::string& task_id
)
{
    if (!user) {
        return USER_NO_AUTH;
    }

    if (!m_db) {
        return UNKNOWN;
    }

    std::unique_lock<std::mutex> lock(m_mutex);

    AnalysisTask task;
    rst_code_e res = m_db->get_analysis_task_by_id(task_id, task);
    if (res != RST_OK) {
        return res;
    }

    if (task.get_user_id() != user->get_useraccountid()) {
        if (logger) logger->warn("OperationAnalysisScheduler::cancel_task - User {} unauthorized to cancel task {} owned by {}",
                                user->get_useraccountid(), task_id, task.get_user_id());
        return USER_NO_AUTH;
    }

    if (task.is_finished()) {
        return RST_OK;
    }

    if (task.get_status() == AnalysisTaskStatus::QUEUED) {
        task.set_status(AnalysisTaskStatus::CANCELLED);
        task.set_stage_description("Cancelled while queued");
        task.set_completed_at(std::time(nullptr));
        m_db->update_analysis_task(task);
        if (logger) logger->info("OperationAnalysisScheduler - Queued task {} cancelled", task_id);
        return RST_OK;
    }

    // Actively running task: signal cancellation token
    auto it = m_active_cancellation_tokens.find(task_id);
    if (it != m_active_cancellation_tokens.end() && it->second) {
        it->second->store(true);
    }

    task.set_status(AnalysisTaskStatus::CANCELLED);
    task.set_stage_description("Cancellation requested by user");
    task.set_completed_at(std::time(nullptr));
    m_db->update_analysis_task(task);

    if (logger) logger->info("OperationAnalysisScheduler - Running task {} marked for cancellation", task_id);
    return RST_OK;
}

rst_code_e OperationAnalysisScheduler::get_task_status(
    const std::shared_ptr<const User>& user,
    const std::string& task_id,
    AnalysisTask& out_task
)
{
    if (!user) {
        return USER_NO_AUTH;
    }

    if (!m_db) {
        return UNKNOWN;
    }

    rst_code_e res = m_db->get_analysis_task_by_id(task_id, out_task);
    if (res != RST_OK) {
        return res;
    }

    if (out_task.get_user_id() != user->get_useraccountid()) {
        return USER_NO_AUTH;
    }

    return RST_OK;
}

rst_code_e OperationAnalysisScheduler::get_user_tasks(
    const std::shared_ptr<const User>& user,
    std::vector<AnalysisTask>& out_tasks
)
{
    if (!user) {
        return USER_NO_AUTH;
    }

    if (!m_db) {
        return UNKNOWN;
    }

    return m_db->get_analysis_tasks_by_user(user->get_useraccountid(), out_tasks);
}

rst_code_e OperationAnalysisScheduler::get_all_tasks(
    const std::shared_ptr<const User>& admin_user,
    std::vector<AnalysisTask>& out_tasks
)
{
    if (!admin_user) {
        return USER_NO_AUTH;
    }

    if (!m_db) {
        return UNKNOWN;
    }

    return m_db->get_all_analysis_tasks(out_tasks);
}

void OperationAnalysisScheduler::set_max_parallel_tasks(size_t max_tasks)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_max_parallel_tasks = (max_tasks == 0) ? 1 : max_tasks;
    m_cv.notify_all();
}

size_t OperationAnalysisScheduler::get_max_parallel_tasks() const
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_max_parallel_tasks;
}

size_t OperationAnalysisScheduler::get_running_tasks_count() const
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_running_task_ids.size();
}

size_t OperationAnalysisScheduler::get_queued_tasks_count() const
{
    if (!m_db) return 0;
    std::vector<AnalysisTask> all;
    if (m_db->get_all_analysis_tasks(all) != RST_OK) return 0;
    size_t count = 0;
    for (const auto& t : all) {
        if (t.get_status() == AnalysisTaskStatus::QUEUED) {
            count++;
        }
    }
    return count;
}

void OperationAnalysisScheduler::set_custom_executor(TaskExecutorFn executor)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_custom_executor = std::move(executor);
}

void OperationAnalysisScheduler::worker_loop()
{
    while (m_running) {
        AnalysisTask task_to_run;
        std::shared_ptr<std::atomic<bool>> cancel_token;

        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this] {
                if (!m_running) return true;
                if (m_running_task_ids.size() >= m_max_parallel_tasks) return false;

                // Check DB for any QUEUED task
                if (!m_db) return false;
                std::vector<AnalysisTask> all;
                if (m_db->get_all_analysis_tasks(all) != RST_OK) return false;
                for (const auto& t : all) {
                    if (t.get_status() == AnalysisTaskStatus::QUEUED &&
                        m_running_task_ids.find(t.get_task_id()) == m_running_task_ids.end()) {
                        return true;
                    }
                }
                return false;
            });

            if (!m_running) {
                break;
            }

            // Find oldest QUEUED task (minimum created_at)
            std::vector<AnalysisTask> all;
            if (m_db->get_all_analysis_tasks(all) == RST_OK) {
                const AnalysisTask* oldest = nullptr;
                for (const auto& t : all) {
                    if (t.get_status() == AnalysisTaskStatus::QUEUED &&
                        m_running_task_ids.find(t.get_task_id()) == m_running_task_ids.end()) {
                        if (!oldest || t.get_created_at() < oldest->get_created_at()) {
                            oldest = &t;
                        }
                    }
                }
                if (oldest) {
                    task_to_run = *oldest;
                }
            }

            if (task_to_run.get_task_id().empty()) {
                continue;
            }

            // Mark running
            task_to_run.set_status(AnalysisTaskStatus::CONVERTING_AUDIO);
            task_to_run.set_started_at(std::time(nullptr));
            task_to_run.set_stage_description("Starting audio conversion & analysis");
            task_to_run.set_progress_percentage(10);
            m_db->update_analysis_task(task_to_run);

            m_running_task_ids.insert(task_to_run.get_task_id());
            cancel_token = std::make_shared<std::atomic<bool>>(false);
            m_active_cancellation_tokens[task_to_run.get_task_id()] = cancel_token;
        }

        execute_single_task(task_to_run, cancel_token);

        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_running_task_ids.erase(task_to_run.get_task_id());
            m_active_cancellation_tokens.erase(task_to_run.get_task_id());
        }

        m_cv.notify_all();
    }
}

void OperationAnalysisScheduler::execute_single_task(
    AnalysisTask task,
    std::shared_ptr<std::atomic<bool>> cancel_token
)
{
    if (cancel_token && cancel_token->load()) {
        task.set_status(AnalysisTaskStatus::CANCELLED);
        task.set_stage_description("Cancelled before stage execution");
        task.set_completed_at(std::time(nullptr));
        if (m_db) m_db->update_analysis_task(task);
        return;
    }

    std::shared_ptr<User> user_obj = std::make_shared<User>("user_" + std::to_string(task.get_user_id()));
    user_obj->set_useraccountid(task.get_user_id());
    user_obj->set_is_authenticated(true);
    std::shared_ptr<const User> user = user_obj;

    std::string execution_id;
    rst_code_e res = RST_OK;

    if (m_custom_executor) {
        res = m_custom_executor(user, task, cancel_token, execution_id);
    } else {
        res = default_execute_coverage(user, task, cancel_token, execution_id);
    }

    if (cancel_token && cancel_token->load()) {
        task.set_status(AnalysisTaskStatus::CANCELLED);
        task.set_stage_description("Task execution cancelled");
        task.set_completed_at(std::time(nullptr));
    } else if (res == RST_OK) {
        task.set_status(AnalysisTaskStatus::COMPLETED);
        task.set_progress_percentage(100);
        task.set_stage_description("Analysis completed successfully");
        task.set_execution_id(execution_id);
        task.set_result_code(RST_OK);
        task.set_completed_at(std::time(nullptr));
    } else {
        task.set_status(AnalysisTaskStatus::FAILED);
        task.set_result_code(res);
        task.set_error_message(get_rst_txt(res));
        task.set_stage_description("Analysis failed with error: " + get_rst_txt(res));
        task.set_completed_at(std::time(nullptr));
    }

    if (m_db) {
        m_db->update_analysis_task(task);
    }
}

rst_code_e OperationAnalysisScheduler::default_execute_coverage(
    const std::shared_ptr<const User>& user,
    AnalysisTask& task,
    std::shared_ptr<std::atomic<bool>> cancel_token,
    std::string& out_execution_id
)
{
    if (!m_coverage_op) {
        if (logger) logger->error("OperationAnalysisScheduler::default_execute_coverage - Coverage operation missing");
        return UNKNOWN;
    }

    if (cancel_token && cancel_token->load()) {
        return TASK_CANCELLED;
    }

    UserConfiguration config;
    if (!task.get_config_snapshot_json().empty()) {
        config.from_json(task.get_config_snapshot_json());
    }

    task.set_status(AnalysisTaskStatus::TRANSCRIBING);
    task.set_stage_description("Transcribing audio and generating embeddings");
    task.set_progress_percentage(40);
    if (m_db) m_db->update_analysis_task(task);

    if (cancel_token && cancel_token->load()) {
        return TASK_CANCELLED;
    }

    rst_code_e res = m_coverage_op->analyze_practice_coverage(
        user,
        task.get_practice_id(),
        config,
        out_execution_id
    );

    return res;
}
