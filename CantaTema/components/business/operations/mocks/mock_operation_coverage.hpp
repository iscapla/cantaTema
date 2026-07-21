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
};

#endif // __MOCK_OPERATION_COVERAGE_HPP
