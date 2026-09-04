/**
 * @file t_operation_coverage_helper.hpp
 * @brief Helper for constructing OperationCoverage with mocks in unit tests.
 */

#ifndef __T_OPERATION_COVERAGE_HELPER_HPP
#define __T_OPERATION_COVERAGE_HELPER_HPP

#include <memory>
#include "operations/operation_coverage.hpp"
#include "../src/operation_coverage_pipeline.hpp"

inline std::unique_ptr<OperationCoverage> make_test_coverage_op(
    std::shared_ptr<IDatabase> db,
    std::shared_ptr<IOperationSubject> subject_op = nullptr,
    std::shared_ptr<IOperationPracticeEvent> practice_op = nullptr,
    std::shared_ptr<IFileHandler> file_handler = nullptr,
    std::shared_ptr<ISpeechRecognition> speech_recognition = nullptr,
    std::shared_ptr<IEmbeddingEngine> embedding_engine = nullptr,
    std::shared_ptr<ISimilaritySearch> similarity_search = nullptr
)
{
    CoveragePipelineDependencies deps;
    deps.db = std::move(db);
    deps.subject_op = std::move(subject_op);
    deps.practice_op = std::move(practice_op);
    deps.file_handler = std::move(file_handler);
    deps.speech_recognition = std::move(speech_recognition);
    deps.embedding_engine = std::move(embedding_engine);
    deps.similarity_search = std::move(similarity_search);
    return OperationCoverage::create_with_pipeline(std::move(deps));
}

#endif // __T_OPERATION_COVERAGE_HELPER_HPP
