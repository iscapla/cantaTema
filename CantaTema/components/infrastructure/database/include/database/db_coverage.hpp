#ifndef __DB_COVERAGE_HPP
#define __DB_COVERAGE_HPP

#include "database/i_database.hpp"
#include <string>

class DB_Coverage : public IDatabase {
public:
    DB_Coverage(void);
    ~DB_Coverage(void) override;

    rst_code_e create_coverage_tables() override;

    rst_code_e save_coverage_analysis_execution(
        int practice_id,
        const std::string& analysis_execution_id,
        double coverage_percentage,
        double speed_score,
        double clarity_score,
        double pacing_score,
        const std::string& whisper_model,
        const std::string& llama_model,
        const std::string& language,
        double similarity_threshold,
        const std::string& config_snapshot_json,
        const std::string& report_json
    ) override;

    rst_code_e get_analysis_executions_for_practice(
        int practice_id,
        std::string& out_executions_list_json
    ) override;

    rst_code_e get_analysis_execution_details(
        const std::string& execution_id,
        std::string& out_report_json,
        std::string& out_config_json
    ) override;

    rst_code_e save_user_configuration(unsigned int user_id, const UserConfiguration& config) override;
    rst_code_e get_user_configuration(unsigned int user_id, UserConfiguration& out_config) override;
};

#endif // __DB_COVERAGE_HPP
