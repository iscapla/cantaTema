/**
 * @file operation_coverage.hpp
 * @brief Public interface and Pimpl facade for Audio-to-PDF coverage analysis.
 */

#ifndef __OPERATION_COVERAGE_HPP
#define __OPERATION_COVERAGE_HPP

#include <memory>
#include <string>
#include <vector>

#include "operations/i_operation_coverage.hpp"

class IOperationSubject;
class IOperationPracticeEvent;
struct CoveragePipelineDependencies;

/**
 * @class OperationCoverage
 * @brief Coordinates the full Audio-to-PDF coverage analysis pipeline.
 *
 * Implements the Pimpl/Pipeline idiom: low-level infrastructure dependencies
 * (database, file handler, speech recognition, embedding engine, similarity search)
 * are encapsulated in the private implementation and not exposed in this header.
 */
class OperationCoverage : public IOperationCoverage
{
public:
    struct Impl;

    /**
     * @brief Constructs OperationCoverage taking ownership of an internal Impl instance.
     * @param impl Unique pointer to the pipeline implementation.
     */
    explicit OperationCoverage(std::unique_ptr<Impl> impl);

    /**
     * @brief Constructs OperationCoverage with injected business operations and default infrastructure.
     * @param subject_op Injected subject business operation.
     * @param practice_op Injected practice event business operation.
     */
    OperationCoverage(
        std::shared_ptr<IOperationSubject> subject_op = nullptr,
        std::shared_ptr<IOperationPracticeEvent> practice_op = nullptr
    );

    /**
     * @brief Factory method for creating an OperationCoverage instance with custom pipeline dependencies.
     * Used primarily by unit tests to inject mocks without leaking infrastructure headers.
     * @param deps Configuration struct containing injected dependencies.
     * @return Unique pointer to configured OperationCoverage.
     */
    static std::unique_ptr<OperationCoverage> create_with_pipeline(CoveragePipelineDependencies deps);

    /**
     * @brief Destructor (defined in .cpp to allow std::unique_ptr<Impl> with incomplete type).
     */
    ~OperationCoverage() override;

    OperationCoverage(OperationCoverage&&) noexcept;
    OperationCoverage& operator=(OperationCoverage&&) noexcept;

    OperationCoverage(const OperationCoverage&) = delete;
    OperationCoverage& operator=(const OperationCoverage&) = delete;

    /**
     * @brief Executes the complete coverage analysis pipeline and logs results in DB.
     */
    rst_code_e analyze_practice_coverage(
        const std::shared_ptr<const User>& user,
        int practice_id,
        const std::string& whisper_model,
        const std::string& llama_model,
        float similarity_threshold,
        const std::string& language,
        std::string& out_analysis_execution_id
    ) override;

    rst_code_e analyze_practice_coverage(
        const std::shared_ptr<const User>& user,
        int practice_id,
        const UserConfiguration& config,
        std::string& out_analysis_execution_id
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

    rst_code_e save_analysis_task(const AnalysisTask& task) override;
    rst_code_e update_analysis_task(const AnalysisTask& task) override;
    rst_code_e get_analysis_task_by_id(const std::string& task_id, AnalysisTask& out_task) override;
    rst_code_e get_analysis_tasks_by_user(unsigned int user_id, std::vector<AnalysisTask>& out_tasks) override;
    rst_code_e get_all_analysis_tasks(std::vector<AnalysisTask>& out_tasks) override;
    rst_code_e get_active_analysis_task_for_practice(int practice_id, AnalysisTask& out_task) override;
    rst_code_e recover_interrupted_analysis_tasks(int max_retries, std::vector<AnalysisTask>& out_recovered_tasks) override;
    rst_code_e delete_analysis_task(const std::string& task_id) override;

private:
    std::unique_ptr<Impl> m_impl;
};

#endif // __OPERATION_COVERAGE_HPP
