#ifndef __OPERATION_COVERAGE_HPP
#define __OPERATION_COVERAGE_HPP

#include <memory>
#include <string>
#include <vector>

#include "operations/i_operation_coverage.hpp"
#include "operations/i_operation_subject.hpp"
#include "operations/i_operation_practice_event.hpp"
#include "database/i_database.hpp"
#include "file_handler/file_handler.hpp"
#include "speech_recognition/i_speech_recognition.hpp"
#include "embeddings/i_embedding_engine.hpp"
#include "similarity/i_similarity_search.hpp"

/**
 * @class OperationCoverage
 * @brief Coordinates the full Audio-to-PDF coverage analysis pipeline.
 */
class OperationCoverage : public IOperationCoverage
{
public:
    /**
     * @brief Constructs OperationCoverage with injected infrastructure & operation dependencies.
     */
    OperationCoverage(
        std::shared_ptr<IDatabase> db = nullptr,
        std::shared_ptr<IOperationSubject> subject_op = nullptr,
        std::shared_ptr<IOperationPracticeEvent> practice_op = nullptr,
        std::shared_ptr<IFileHandler> file_handler = nullptr,
        std::shared_ptr<ISpeechRecognition> speech_recognition = nullptr,
        std::shared_ptr<IEmbeddingEngine> embedding_engine = nullptr,
        std::shared_ptr<ISimilaritySearch> similarity_search = nullptr
    );

    ~OperationCoverage() override = default;

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

private:
    std::shared_ptr<IDatabase> m_db;
    std::shared_ptr<IOperationSubject> m_subject_op;
    std::shared_ptr<IOperationPracticeEvent> m_practice_op;
    std::shared_ptr<IFileHandler> m_file_handler;
    std::shared_ptr<ISpeechRecognition> m_speech_recognition;
    std::shared_ptr<IEmbeddingEngine> m_embedding_engine;
    std::shared_ptr<ISimilaritySearch> m_similarity_search;

    std::string generate_execution_uuid() const;
};

#endif // __OPERATION_COVERAGE_HPP
