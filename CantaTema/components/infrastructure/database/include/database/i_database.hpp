#ifndef __I_DATABASE_HPP
#define __I_DATABASE_HPP

#include <string>
#include "primitives/definitions.hpp"

class IDatabase {
public:
    virtual ~IDatabase() = default;
    
    virtual rst_code_e create_coverage_tables() = 0;
    
    virtual rst_code_e save_coverage_analysis_execution(
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
    ) = 0;
    
    virtual rst_code_e get_analysis_executions_for_practice(
        int practice_id,
        std::string& out_executions_list_json
    ) = 0;
    
    virtual rst_code_e get_analysis_execution_details(
        const std::string& execution_id,
        std::string& out_report_json,
        std::string& out_config_json
    ) = 0;
};

#endif // __I_DATABASE_HPP
