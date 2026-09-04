/**
 * @file operation_coverage_pipeline.hpp
 * @brief Internal pipeline implementation and dependencies for OperationCoverage (Pimpl idiom).
 *
 * This header contains the full dependency injection definitions and the internal
 * execution pipeline. It is only included by operation_coverage.cpp and unit tests
 * that inject mock infrastructure.
 */

#ifndef __OPERATION_COVERAGE_PIPELINE_HPP
#define __OPERATION_COVERAGE_PIPELINE_HPP

#include <memory>
#include <string>
#include <vector>

#include "operations/operation_coverage.hpp"
#include "operations/i_operation_subject.hpp"
#include "operations/i_operation_practice_event.hpp"
#include "database/i_database.hpp"
#include "file_handler/i_file_handler.hpp"
#include "speech_recognition/i_speech_recognition.hpp"
#include "embeddings/i_embedding_engine.hpp"
#include "similarity/i_similarity_search.hpp"

/**
 * @struct CoveragePipelineDependencies
 * @brief Holds the low-level infrastructure and business operation dependencies
 * for the Audio-to-PDF coverage analysis pipeline.
 */
struct CoveragePipelineDependencies {
    std::shared_ptr<IDatabase> db;
    std::shared_ptr<IOperationSubject> subject_op;
    std::shared_ptr<IOperationPracticeEvent> practice_op;
    std::shared_ptr<IFileHandler> file_handler;
    std::shared_ptr<ISpeechRecognition> speech_recognition;
    std::shared_ptr<IEmbeddingEngine> embedding_engine;
    std::shared_ptr<ISimilaritySearch> similarity_search;
};

/**
 * @struct OperationCoverage::Impl
 * @brief Concrete internal state and pipeline execution engine for OperationCoverage.
 */
struct OperationCoverage::Impl {
    std::shared_ptr<IDatabase> m_db;
    std::shared_ptr<IOperationSubject> m_subject_op;
    std::shared_ptr<IOperationPracticeEvent> m_practice_op;
    std::shared_ptr<IFileHandler> m_file_handler;
    std::shared_ptr<ISpeechRecognition> m_speech_recognition;
    std::shared_ptr<IEmbeddingEngine> m_embedding_engine;
    std::shared_ptr<ISimilaritySearch> m_similarity_search;

    explicit Impl(CoveragePipelineDependencies deps);

    std::string generate_execution_uuid() const;

    rst_code_e analyze_practice_coverage(
        const std::shared_ptr<const User>& user,
        int practice_id,
        const UserConfiguration& config,
        std::string& out_analysis_execution_id
    );

    rst_code_e analyze_practice_coverage(
        const std::shared_ptr<const User>& user,
        int practice_id,
        const std::string& whisper_model,
        const std::string& llama_model,
        float similarity_threshold,
        const std::string& language,
        std::string& out_analysis_execution_id
    );

    rst_code_e get_analysis_executions_for_practice(
        int practice_id,
        std::string& out_executions_list_json
    );

    rst_code_e get_analysis_execution_details(
        const std::string& execution_id,
        std::string& out_report_json,
        std::string& out_config_json
    );

    rst_code_e save_analysis_task(const AnalysisTask& task);
    rst_code_e update_analysis_task(const AnalysisTask& task);
    rst_code_e get_analysis_task_by_id(const std::string& task_id, AnalysisTask& out_task);
    rst_code_e get_analysis_tasks_by_user(unsigned int user_id, std::vector<AnalysisTask>& out_tasks);
    rst_code_e get_all_analysis_tasks(std::vector<AnalysisTask>& out_tasks);
    rst_code_e get_active_analysis_task_for_practice(int practice_id, AnalysisTask& out_task);
    rst_code_e recover_interrupted_analysis_tasks(int max_retries, std::vector<AnalysisTask>& out_recovered_tasks);
    rst_code_e delete_analysis_task(const std::string& task_id);
};

#endif // __OPERATION_COVERAGE_PIPELINE_HPP
