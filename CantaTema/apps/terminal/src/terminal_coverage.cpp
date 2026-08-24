#include <iomanip>
#include <iostream>
#include <sstream>
#include "terminal/terminal_session.hpp"
#include "models/manager_models.hpp"
#include "primitives/utils_logger.hpp"
#include "primitives/utils_prints.hpp"

void TerminalSession::models_list(std::ostream &out)
{
    std::vector<ManagerModels::ModelInfo> whisper_models;
    std::vector<ManagerModels::ModelInfo> llama_models;

    op->models_get_whisper(true, whisper_models);
    op->models_get_llama(true, llama_models);

    out << "--- Whisper Speech Recognition Models ---" << std::endl;
    out << std::left << std::setw(25) << "Model Name" << std::setw(10) << "Local" << std::setw(10) << "Remote" << "Path" << std::endl;
    out << std::string(100, '-') << std::endl;

    for (const auto &model : whisper_models) {
        out << std::left << std::setw(25) << model.name << std::setw(10) << (model.available_local ? "YES" : "NO") << std::setw(10) << (model.available_network ? "YES" : "NO") << UtilsPrints::format_path_for_display(model.path) << std::endl;
    }

    out << std::endl << "--- Llama Embedding Models ---" << std::endl;
    out << std::left << std::setw(25) << "Model Name" << std::setw(10) << "Local" << std::setw(10) << "Remote" << "Path" << std::endl;
    out << std::string(100, '-') << std::endl;

    for (const auto &model : llama_models) {
        out << std::left << std::setw(25) << model.name << std::setw(10) << (model.available_local ? "YES" : "NO") << std::setw(10) << (model.available_network ? "YES" : "NO") << UtilsPrints::format_path_for_display(model.path) << std::endl;
    }
}

void TerminalSession::models_download_whisper(std::ostream &out, const std::string &model_name)
{
    out << "Downloading Whisper model '" << model_name << "'..." << std::endl;
    rst_code_e rst = op->models_download_whisper(model_name, [&out](const DownloadProgress &progress) {
        if (progress.total_bytes > 0) {
            float percent = (float)progress.downloaded_bytes / (float)progress.total_bytes * 100.0f;
            out << "\rDownloading " << progress.file_name << ": " << (int)percent << "%" << std::flush;
        }
    });
    out << std::endl;
    if (rst != RST_OK) {
        logger->error("Failed to download Whisper model: {}", get_rst_txt(rst));
        out << "Failed to download Whisper model: " << get_rst_txt(rst) << std::endl;
    } else {
        logger->info("Whisper model '{}' downloaded successfully", model_name);
        out << "Whisper model '" << model_name << "' downloaded successfully." << std::endl;
    }
}

void TerminalSession::models_download_llama(std::ostream &out, const std::string &model_name)
{
    out << "Downloading Llama embedding model '" << model_name << "'..." << std::endl;
    rst_code_e rst = op->models_download_llama(model_name, [&out](const DownloadProgress &progress) {
        if (progress.total_bytes > 0) {
            float percent = (float)progress.downloaded_bytes / (float)progress.total_bytes * 100.0f;
            out << "\rDownloading " << progress.file_name << ": " << (int)percent << "%" << std::flush;
        }
    });
    out << std::endl;
    if (rst != RST_OK) {
        logger->error("Failed to download Llama model: {}", get_rst_txt(rst));
        out << "Failed to download Llama model: " << get_rst_txt(rst) << std::endl;
    } else {
        logger->info("Llama model '{}' downloaded successfully", model_name);
        out << "Llama model '" << model_name << "' downloaded successfully." << std::endl;
    }
}

void TerminalSession::models_remove_whisper(std::ostream &out, const std::string &model_name)
{
    rst_code_e rst = op->models_remove_whisper(model_name);
    if (rst != RST_OK) {
        logger->error("Failed to remove Whisper model '{}': {}", model_name, get_rst_txt(rst));
        out << "Failed to remove Whisper model: " << get_rst_txt(rst) << std::endl;
    } else {
        logger->info("Whisper model '{}' removed successfully", model_name);
        out << "Whisper model '" << model_name << "' removed successfully." << std::endl;
    }
}

void TerminalSession::models_remove_llama(std::ostream &out, const std::string &model_name)
{
    rst_code_e rst = op->models_remove_llama(model_name);
    if (rst != RST_OK) {
        logger->error("Failed to remove Llama model '{}': {}", model_name, get_rst_txt(rst));
        out << "Failed to remove Llama model: " << get_rst_txt(rst) << std::endl;
    } else {
        logger->info("Llama model '{}' removed successfully", model_name);
        out << "Llama model '" << model_name << "' removed successfully." << std::endl;
    }
}

void TerminalSession::coverage_analyze(
    std::ostream &out,
    unsigned int practice_id,
    const std::string &whisper_model,
    const std::string &llama_model,
    float similarity_threshold,
    const std::string &language
)
{
    std::string task_id;
    out << "Submitting coverage analysis task for practice ID " << practice_id << " to scheduler..." << std::endl;

    UserConfiguration config;
    if (!whisper_model.empty()) {
        config.whisper.model_name = whisper_model;
    }
    if (!llama_model.empty()) {
        config.comparison.embedding_model_name = llama_model;
    }
    if (similarity_threshold > 0.0f) {
        config.comparison.similarity_threshold = similarity_threshold;
    }
    if (!language.empty()) {
        config.whisper.language = language;
    }

    rst_code_e rst = op->analysis_task_submit(static_cast<int>(practice_id), task_id, config);

    if (rst == RST_OK) {
        logger->info("Coverage analysis task queued: {}", task_id);
        out << "Analysis task successfully queued!" << std::endl;
        out << "Task ID: " << task_id << std::endl;
        out << "Use 'coverage status " << task_id << "' (or 'coverage task_status " << task_id << "') to track progress." << std::endl;
    } else if (rst == TASK_ALREADY_QUEUED) {
        logger->warn("Practice ID {} already has active task: {}", practice_id, task_id);
        out << "Practice ID " << practice_id << " already has an active task: " << task_id << std::endl;
        out << "Use 'coverage status " << task_id << "' to check status." << std::endl;
    } else {
        logger->error("Failed to submit analysis task for practice {}: {}", practice_id, get_rst_txt(rst));
        out << "Failed to submit analysis task: " << get_rst_txt(rst) << std::endl;
    }
}

void TerminalSession::coverage_report(std::ostream &out, const std::string &execution_id)
{
    std::string report_json, config_json;
    rst_code_e rst = op->get_analysis_execution_details(execution_id, report_json, config_json);

    if (rst != RST_OK) {
        logger->error("Failed to retrieve analysis report for ID {}: {}", execution_id, get_rst_txt(rst));
        out << "Error retrieving report: " << get_rst_txt(rst) << std::endl;
    } else {
        out << "=== COVERAGE ANALYSIS REPORT ===" << std::endl;
        out << "Execution ID: " << execution_id << std::endl;
        out << "--- Report JSON ---" << std::endl;
        out << report_json << std::endl;
        out << "--- Configuration Snapshot ---" << std::endl;
        out << config_json << std::endl;
    }
}

void TerminalSession::coverage_report_by_practice(std::ostream &out, unsigned int practice_id)
{
    std::string report_json, config_json;
    rst_code_e rst = op->get_analysis_execution_details_by_practice(practice_id, report_json, config_json);
    if (rst != RST_OK) {
        out << "No analysis report found for practice ID " << practice_id << " (Error: " << get_rst_txt(rst) << ")." << std::endl;
        return;
    }

    out << "=== COVERAGE ANALYSIS REPORT ===" << std::endl;
    out << "Practice ID: " << practice_id << std::endl;
    out << "--- Report JSON ---" << std::endl;
    out << report_json << std::endl;
    out << "--- Configuration Snapshot ---" << std::endl;
    out << config_json << std::endl;
}

void TerminalSession::coverage_task_submit(std::ostream &out, unsigned int practice_id)
{
    std::string task_id;
    out << "Submitting analysis task for practice ID " << practice_id << " to scheduler..." << std::endl;
    rst_code_e rst = op->analysis_task_submit(static_cast<int>(practice_id), task_id);
    if (rst == RST_OK) {
        logger->info("Analysis task submitted: {}", task_id);
        out << "Task successfully queued!" << std::endl;
        out << "Task ID: " << task_id << std::endl;
        out << "Use 'coverage task_status " << task_id << "' to monitor progress." << std::endl;
    } else if (rst == TASK_ALREADY_QUEUED) {
        logger->warn("Practice ID {} already has active task: {}", practice_id, task_id);
        out << "Practice ID " << practice_id << " already has an active task: " << task_id << std::endl;
        out << "Use 'coverage task_status " << task_id << "' to check status." << std::endl;
    } else {
        logger->error("Failed to submit analysis task for practice {}: {}", practice_id, get_rst_txt(rst));
        out << "Failed to submit task: " << get_rst_txt(rst) << std::endl;
    }
}

void TerminalSession::coverage_task_status(std::ostream &out, const std::string &task_id)
{
    AnalysisTask task;
    rst_code_e rst = op->analysis_task_get_status(task_id, task);
    if (rst != RST_OK) {
        logger->error("Failed to query task {}: {}", task_id, get_rst_txt(rst));
        out << "Task not found or access denied: " << get_rst_txt(rst) << std::endl;
        return;
    }

    out << "=== ANALYSIS TASK STATUS ===" << std::endl;
    out << "Task ID:       " << task.get_task_id() << std::endl;
    out << "Practice ID:   " << task.get_practice_id() << std::endl;
    out << "Status:        " << AnalysisTask::status_to_string(task.get_status()) << std::endl;
    out << "Progress:      " << task.get_progress_percentage() << "%" << std::endl;
    out << "Current Stage: " << task.get_stage_description() << std::endl;
    if (!task.get_execution_id().empty()) {
        out << "Execution ID:  " << task.get_execution_id() << std::endl;
        out << "View Report:   coverage report " << task.get_execution_id() << std::endl;
    }
    if (!task.get_error_message().empty()) {
        out << "Error:         " << task.get_error_message() << std::endl;
    }
    if (task.get_retry_count() > 0) {
        out << "Retries:       " << task.get_retry_count() << std::endl;
    }
}

void TerminalSession::coverage_task_cancel(std::ostream &out, const std::string &task_id)
{
    out << "Requesting cancellation for task " << task_id << "..." << std::endl;
    rst_code_e rst = op->analysis_task_cancel(task_id);
    if (rst == RST_OK) {
        logger->info("Task {} cancelled successfully", task_id);
        out << "Task " << task_id << " cancelled successfully." << std::endl;
    } else {
        logger->error("Failed to cancel task {}: {}", task_id, get_rst_txt(rst));
        out << "Failed to cancel task: " << get_rst_txt(rst) << std::endl;
    }
}

void TerminalSession::coverage_task_list(std::ostream &out)
{
    std::vector<AnalysisTask> tasks;
    rst_code_e rst = op->analysis_task_get_user_tasks(tasks);
    if (rst != RST_OK) {
        out << "Failed to retrieve user tasks: " << get_rst_txt(rst) << std::endl;
        return;
    }

    out << "=== YOUR ANALYSIS TASKS ===" << std::endl;
    if (tasks.empty()) {
        out << "No analysis tasks found." << std::endl;
        return;
    }

    out << std::left << std::setw(30) << "Task ID" 
        << std::setw(12) << "Practice" 
        << std::setw(22) << "Status" 
        << std::setw(10) << "Progress" 
        << "Stage" << std::endl;
    out << std::string(100, '-') << std::endl;

    for (const auto& t : tasks) {
        out << std::left << std::setw(30) << t.get_task_id()
            << std::setw(12) << t.get_practice_id()
            << std::setw(22) << AnalysisTask::status_to_string(t.get_status())
            << std::setw(10) << (std::to_string(t.get_progress_percentage()) + "%")
            << t.get_stage_description() << std::endl;
    }
}

void TerminalSession::coverage_admin_tasks(std::ostream &out)
{
    std::vector<AnalysisTask> tasks;
    rst_code_e rst = op->analysis_task_get_all_tasks(tasks);
    if (rst != RST_OK) {
        out << "Failed to retrieve all tasks: " << get_rst_txt(rst) << std::endl;
        return;
    }

    out << "=== ALL SYSTEM ANALYSIS TASKS (ADMIN) ===" << std::endl;
    if (tasks.empty()) {
        out << "No tasks found in scheduler system." << std::endl;
        return;
    }

    out << std::left << std::setw(30) << "Task ID" 
        << std::setw(10) << "User ID"
        << std::setw(12) << "Practice" 
        << std::setw(22) << "Status" 
        << std::setw(10) << "Progress" 
        << "Stage / Result" << std::endl;
    out << std::string(110, '-') << std::endl;

    for (const auto& t : tasks) {
        out << std::left << std::setw(30) << t.get_task_id()
            << std::setw(10) << t.get_user_id()
            << std::setw(12) << t.get_practice_id()
            << std::setw(22) << AnalysisTask::status_to_string(t.get_status())
            << std::setw(10) << (std::to_string(t.get_progress_percentage()) + "%")
            << t.get_stage_description() << std::endl;
    }
}

