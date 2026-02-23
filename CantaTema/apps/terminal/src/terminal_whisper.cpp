#include <iomanip>

#include "terminal/terminal_session.hpp"
#include "models/manager_whisper.hpp"

void TerminalSession::whisper_get_available_models(std::ostream &out)
{
    ManagerWhisper manager;
    std::vector<ManagerWhisper::WhisperModel> models;
    manager.get_available_models(true, models);

    out << std::left << std::setw(20) << "Model Name" << std::setw(10) << "Local" << std::setw(10) << "Remote" << "Path" << std::endl;
    out << std::string(100, '-') << std::endl;

    for (const auto &model : models) {
        out << std::left << std::setw(20) << model.name << std::setw(10) << (model.available_local ? "YES" : "NO") << std::setw(10) << (model.available_network ? "YES" : "NO") << model.path << std::endl;
    }
}

void TerminalSession::whisper_download_model(std::ostream &out, const std::string &c)
{
    ManagerWhisper manager;
    manager.network_download_model(c, [&out](const DownloadProgress &progress) {
        if (progress.total_bytes > 0) {
            float percent = (float)progress.downloaded_bytes / (float)progress.total_bytes * 100.0f;

            out << "\rDownloading " << progress.file_name << ": " << (int)percent << "%" << std::flush;
        }
    });
    out << std::endl;
}
