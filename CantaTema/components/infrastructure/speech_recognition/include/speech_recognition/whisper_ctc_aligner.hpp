/**
 * @file whisper_ctc_aligner.hpp
 * @brief Whisper-based CTC forced aligner implementation.
 */

#ifndef WHISPER_CTC_ALIGNER_HPP
#define WHISPER_CTC_ALIGNER_HPP

#include "speech_recognition/i_ctc_aligner.hpp"

/**
 * @class WhisperCtcAligner
 * @brief Implementation of ICtcAligner using Viterbi matrix path alignment over audio frames to tighten word/sentence timecodes.
 */
class WhisperCtcAligner : public ICtcAligner {
public:
    WhisperCtcAligner() = default;
    ~WhisperCtcAligner() override = default;

    std::string get_aligner_id() const override;

    std::vector<AlignedWordToken> align_tokens(
        const std::vector<float>& pcm_samples,
        int sample_rate,
        const std::vector<std::string>& tokens
    ) override;
};

#endif // WHISPER_CTC_ALIGNER_HPP
