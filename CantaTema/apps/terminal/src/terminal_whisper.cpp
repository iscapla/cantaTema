#include <iomanip>

#include "terminal/terminal_session.hpp"
#include "models/manager_models.hpp"

void TerminalSession::whisper_get_available_models(std::ostream &out)
{
    std::vector<ManagerModels::ModelInfo> models;
    op->models_get_whisper(true, models);

    out << std::left << std::setw(25) << "Model Name" << std::setw(10) << "Local" << std::setw(10) << "Remote" << "Path" << std::endl;
    out << std::string(100, '-') << std::endl;

    for (const auto &model : models) {
        out << std::left << std::setw(25) << model.name << std::setw(10) << (model.available_local ? "YES" : "NO") << std::setw(10) << (model.available_network ? "YES" : "NO") << model.path << std::endl;
    }
}

void TerminalSession::whisper_download_model(std::ostream &out, const std::string &c)
{
    op->models_download_whisper(c, [&out](const DownloadProgress &progress) {
        if (progress.total_bytes > 0) {
            float percent = (float)progress.downloaded_bytes / (float)progress.total_bytes * 100.0f;

            out << "\rDownloading " << progress.file_name << ": " << (int)percent << "%" << std::flush;
        }
    });
    out << std::endl;
}
