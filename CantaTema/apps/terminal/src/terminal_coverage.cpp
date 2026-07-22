#include <iomanip>
#include <iostream>
#include <sstream>
#include "terminal/terminal_session.hpp"
#include "models/manager_models.hpp"
#include "primitives/utils_logger.hpp"

void TerminalSession::models_get_available(std::ostream &out)
{
    ManagerModels manager;
    std::vector<ManagerModels::ModelInfo> whisper_models;
    std::vector<ManagerModels::ModelInfo> llama_models;

    manager.get_whisper_models(true, whisper_models);
    manager.get_llama_models(true, llama_models);

    out << "--- Whisper Speech Recognition Models ---" << std::endl;
    out << std::left << std::setw(25) << "Model Name" << std::setw(10) << "Local" << std::setw(10) << "Remote" << "Path" << std::endl;
    out << std::string(100, '-') << std::endl;

    for (const auto &model : whisper_models) {
        out << std::left << std::setw(25) << model.name << std::setw(10) << (model.available_local ? "YES" : "NO") << std::setw(10) << (model.available_network ? "YES" : "NO") << model.path << std::endl;
    }

    out << std::endl << "--- Llama Embedding Models ---" << std::endl;
    out << std::left << std::setw(25) << "Model Name" << std::setw(10) << "Local" << std::setw(10) << "Remote" << "Path" << std::endl;
    out << std::string(100, '-') << std::endl;

    for (const auto &model : llama_models) {
        out << std::left << std::setw(25) << model.name << std::setw(10) << (model.available_local ? "YES" : "NO") << std::setw(10) << (model.available_network ? "YES" : "NO") << model.path << std::endl;
    }
}

void TerminalSession::models_download(std::ostream &out, const std::string &model_type_str, const std::string &model_name)
{
    ManagerModels manager;
    ModelType type = ModelType::Whisper;
    if (model_type_str == "llama" || model_type_str == "embedding" || model_type_str == "embeddings") {
        type = ModelType::Llama;
    }

    manager.network_download_model(type, model_name, [&out](const DownloadProgress &progress) {
        if (progress.total_bytes > 0) {
            float percent = (float)progress.downloaded_bytes / (float)progress.total_bytes * 100.0f;
            out << "\rDownloading " << progress.file_name << ": " << (int)percent << "%" << std::flush;
        }
    });
    out << std::endl;
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
        out << "Use 'coverage report " << out_execution_id << "' to view detailed report." << std::endl;
    }
}

void TerminalSession::coverage_history(std::ostream &out, unsigned int practice_id)
{
    std::string json_list;
    rst_code_e rst = op->get_analysis_executions_for_practice(practice_id, json_list);

    if (rst != RST_OK) {
        logger->error("Failed to retrieve analysis history: {}", get_rst_txt(rst));
        out << "Error retrieving history: " << get_rst_txt(rst) << std::endl;
    } else {
        out << "Analysis History for Practice ID " << practice_id << ":" << std::endl;
        out << json_list << std::endl;
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
