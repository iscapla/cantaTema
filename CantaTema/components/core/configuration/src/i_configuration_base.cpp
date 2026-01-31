#include "configuration/i_configuration_base.hpp"
#include <fstream>
#include "primitives/utils_logger.hpp"

IConfigurationBase::IConfigurationBase() {
    ini.SetUnicode();
    ini.SetMultiLine(true);
}

IConfigurationBase::~IConfigurationBase() {}

rst_code_e IConfigurationBase::parse(const std::string &encryption_key) {
    
    std::lock_guard<std::mutex> lock(mtx);
    this->encryption_key = encryption_key;
    
    if (!std::filesystem::exists(file_path)) {
        if (file_path.has_parent_path()) {
            std::filesystem::create_directories(file_path.parent_path());
        }
        std::ofstream ofs(file_path);
        if (ofs.is_open()) {
            logger->warn("Configuration file not found. Created empty file: {}", file_path.string());
            ofs.close();
        } else {
            logger->error("Failed to create configuration file: {}", file_path.string());
        }
    }

    ini.Reset();
    SI_Error rc = ini.LoadFile(file_path.c_str());
    if (rc < 0) return UNKNOWN;
    
    return RST_OK;
}

rst_code_e IConfigurationBase::update_values_to_file() {
    std::lock_guard<std::mutex> lock(mtx);
    if (file_path.empty()) return UNKNOWN;

    SI_Error rc = ini.SaveFile(file_path.c_str());
    if (rc < 0) return UNKNOWN;
    
    return RST_OK;
}

rst_code_e IConfigurationBase::set_default_if_not_present(const std::string &section, const std::string &field, const std::string &value) {
    std::lock_guard<std::mutex> lock(mtx);
    const char* current_val = ini.GetValue(section.c_str(), field.c_str(), nullptr);

    if (current_val == nullptr) {
        SI_Error rc = ini.SetValue(section.c_str(), field.c_str(), value.c_str());
        if (rc < 0) return UNKNOWN;
    }
    return RST_OK;
}

std::string IConfigurationBase::get(const std::string &section, const std::string &field) {
    std::lock_guard<std::mutex> lock(mtx);
    const char* value = ini.GetValue(section.c_str(), field.c_str(), nullptr);
    return value ? std::string(value) : "";
}

rst_code_e IConfigurationBase::set(const std::string &section, const std::string &field, const std::string &value) {
    std::lock_guard<std::mutex> lock(mtx);
    SI_Error rc = ini.SetValue(section.c_str(), field.c_str(), value.c_str());
    if (rc < 0) return UNKNOWN;
    return RST_OK;
}

void IConfigurationBase::set_file_path(const std::filesystem::path &file_path) {
    std::lock_guard<std::mutex> lock(mtx);
    this->file_path = file_path;
}
