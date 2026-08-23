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
    std::string out_execution_id;
    out << "Starting coverage analysis for practice ID: " << practice_id << "..." << std::endl;

    rst_code_e rst = op->analyze_practice_coverage(
        practice_id,
        out_execution_id,
        whisper_model,
        llama_model,
        similarity_threshold,
        language
    );

    if (rst != RST_OK) {
        logger->error("Coverage analysis failed: {}", get_rst_txt(rst));
        out << "Coverage analysis failed: " << get_rst_txt(rst) << std::endl;
    } else {
        logger->info("Coverage analysis completed successfully. Execution ID: {}", out_execution_id);
        out << "Coverage analysis completed! Analysis Execution ID: " << out_execution_id << std::endl;
        out << "Practice ID " << practice_id << " is now linked to analysis execution: " << out_execution_id << std::endl;
        out << "Use 'coverage report " << out_execution_id << "' to view detailed report." << std::endl;
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
