#ifndef __MOCK_OPERATION_COVERAGE_HPP
#define __MOCK_OPERATION_COVERAGE_HPP

#include <gmock/gmock.h>
#include "operations/i_operation_coverage.hpp"

class MockOperationCoverage : public IOperationCoverage {
public:
    MOCK_METHOD(rst_code_e, analyze_practice_coverage, (
        const std::shared_ptr<const User>& user,
        int practice_id,
        const std::string& whisper_model,
        const std::string& llama_model,
        float similarity_threshold,
        const std::string& language,
        std::string& out_analysis_execution_id
    ), (override));

    MOCK_METHOD(rst_code_e, analyze_practice_coverage, (
        const std::shared_ptr<const User>& user,
        int practice_id,
        const UserConfiguration& config,
        std::string& out_analysis_execution_id
    ), (override));

    MOCK_METHOD(rst_code_e, get_analysis_executions_for_practice, (
        int practice_id,
        std::string& out_executions_list_json
    ), (override));

    MOCK_METHOD(rst_code_e, get_analysis_execution_details, (
        const std::string& execution_id,
        std::string& out_report_json,
        std::string& out_config_json
    ), (override));

    MOCK_METHOD(rst_code_e, save_analysis_task, (const AnalysisTask& task), (override));
    MOCK_METHOD(rst_code_e, update_analysis_task, (const AnalysisTask& task), (override));
    MOCK_METHOD(rst_code_e, get_analysis_task_by_id, (const std::string& task_id, AnalysisTask& out_task), (override));
    MOCK_METHOD(rst_code_e, get_analysis_tasks_by_user, (unsigned int user_id, std::vector<AnalysisTask>& out_tasks), (override));
    MOCK_METHOD(rst_code_e, get_all_analysis_tasks, (std::vector<AnalysisTask>& out_tasks), (override));
    MOCK_METHOD(rst_code_e, get_active_analysis_task_for_practice, (int practice_id, AnalysisTask& out_task), (override));
    MOCK_METHOD(rst_code_e, recover_interrupted_analysis_tasks, (int max_retries, std::vector<AnalysisTask>& out_recovered_tasks), (override));
    MOCK_METHOD(rst_code_e, delete_analysis_task, (const std::string& task_id), (override));
};

#endif // __MOCK_OPERATION_COVERAGE_HPP
