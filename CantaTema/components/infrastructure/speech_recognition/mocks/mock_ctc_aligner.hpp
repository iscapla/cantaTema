/**
 * @file mock_ctc_aligner.hpp
 * @brief GoogleMock implementation of ICtcAligner interface for unit testing.
 */

#ifndef MOCK_CTC_ALIGNER_HPP
#define MOCK_CTC_ALIGNER_HPP

#include <gmock/gmock.h>
#include "speech_recognition/i_ctc_aligner.hpp"

class MockCtcAligner : public ICtcAligner {
public:
    MOCK_METHOD(std::string, get_aligner_id, (), (const, override));
    MOCK_METHOD(std::vector<AlignedWordToken>, align_tokens, (
        const std::vector<float>& pcm_samples,
        int sample_rate,
        const std::vector<std::string>& tokens
    ), (override));
};

#endif // MOCK_CTC_ALIGNER_HPP
