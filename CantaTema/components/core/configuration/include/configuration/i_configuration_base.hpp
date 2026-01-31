#ifndef CONFIGURATION_BASE_HPP
#define CONFIGURATION_BASE_HPP

#include <string>
#include <mutex>
#include <filesystem>

#include "SimpleIni.h"

#include "primitives/definitions.hpp"

class IConfigurationBase {
protected:
    IConfigurationBase();
    ~IConfigurationBase();

    rst_code_e parse(const std::string &encryption_key=std::string());
    rst_code_e update_values_to_file();

    rst_code_e set_default_if_not_present(const std::string &section, const std::string &field, const std::string &value);

    void set_file_path(const std::filesystem::path &file_path);
    std::string get(const std::string &section, const std::string &field);
    rst_code_e set(const std::string &section, const std::string &field, const std::string &value);

private:
    std::string encryption_key;
    std::filesystem::path file_path;
    CSimpleIniA ini;
    std::mutex mtx;
};

#endif // CONFIGURATION_BASE_HPP