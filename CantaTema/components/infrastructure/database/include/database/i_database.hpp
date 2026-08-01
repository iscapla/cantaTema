/**
 * @file i_database.hpp
 * @brief Abstract interface for database storage of metrics, configurations, and coverage analysis records.
 */

#ifndef __I_DATABASE_HPP
#define __I_DATABASE_HPP

#include <string>
#include "primitives/definitions.hpp"
#include "primitives/user_configuration.hpp"

/**
 * @class IDatabase
 * @brief Abstract repository interface for database persistence and querying operations.
 */
class IDatabase {
public:
    /**
     * @brief Virtual destructor for IDatabase.
     */
    virtual ~IDatabase() = default;
    
    /**
     * @brief Creates coverage analysis and user configuration tables in SQLite database.
     * @return rst_code_e RST_OK on success, or error code.
     */
    virtual rst_code_e create_coverage_tables() = 0;
    
    /**
     * @brief Persists coverage analysis execution metrics and report JSON payload.
     * @param practice_id Identifier of associated practice event.
     * @param analysis_execution_id Unique GUID string of execution analysis.
     * @param coverage_percentage Overall reference coverage percentage.
     * @param speed_score Voice speed metric score (0-100).
     * @param clarity_score Voice clarity metric score (0-100).
     * @param pacing_score Voice pacing metric score (0-100).
     * @param whisper_model Model name identifier for speech recognition.
     * @param llama_model Model name identifier for embedding generation.
     * @param language Spoken language ISO code.
     * @param similarity_threshold Threshold used for matching.
     * @param config_snapshot_json JSON snapshot of UserConfiguration used during analysis.
     * @param report_json Full generated report JSON payload.
     * @return rst_code_e RST_OK on success, or error code.
     */
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
    
    /**
     * @brief Retrieves JSON list of all analysis executions for a given practice event.
     * @param practice_id Practice event identifier.
     * @param out_executions_list_json Output string receiving JSON array.
     * @return rst_code_e RST_OK on success, or error code.
     */
    virtual rst_code_e get_analysis_executions_for_practice(
        int practice_id,
        std::string& out_executions_list_json
    ) = 0;
    
    /**
     * @brief Retrieves detailed report and configuration JSON for a specific analysis execution ID.
     * @param execution_id Unique GUID string of execution analysis.
     * @param out_report_json Output string receiving report JSON payload.
     * @param out_config_json Output string receiving configuration snapshot JSON payload.
     * @return rst_code_e RST_OK on success, or error code.
     */
    virtual rst_code_e get_analysis_execution_details(
        const std::string& execution_id,
        std::string& out_report_json,
        std::string& out_config_json
    ) = 0;

    /**
     * @brief Persists UserConfiguration for a specific user ID.
     * @param user_id User identifier.
     * @param config Configuration parameters struct.
     * @return rst_code_e RST_OK on success, or error code.
     */
    virtual rst_code_e save_user_configuration(unsigned int user_id, const UserConfiguration& config) = 0;

    /**
     * @brief Loads UserConfiguration for a specific user ID.
     * @param user_id User identifier.
     * @param out_config Output struct receiving configuration parameters.
     * @return rst_code_e RST_OK on success, or error code.
     */
    virtual rst_code_e get_user_configuration(unsigned int user_id, UserConfiguration& out_config) = 0;
};

#endif // __I_DATABASE_HPP
