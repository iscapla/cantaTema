/**
 * @file i_ctc_aligner.hpp
 * @brief Abstract interface for Connectionist Temporal Classification (CTC) forced aligners.
 */

#ifndef I_CTC_ALIGNER_HPP
#define I_CTC_ALIGNER_HPP

#include <string>
#include <vector>
#include <cstdint>

/**
 * @struct AlignedWordToken
 * @brief Word token with millisecond timecode boundaries and alignment confidence score.
 */
struct AlignedWordToken {
    std::string word;
    int64_t start_ms = 0;
    int64_t end_ms = 0;
    float confidence = 1.0f;
};

/**
 * @class ICtcAligner
 * @brief Abstract interface defining 2nd-pass CTC forced alignment over audio PCM samples to eliminate timestamp drift.
 */
class ICtcAligner {
public:
    virtual ~ICtcAligner() = default;

    /**
     * @brief Retrieves the unique identifier for this aligner (e.g. "whisper_ctc", "wav2vec2_ctc").
     * @return std::string Aligner ID.
     */
    virtual std::string get_aligner_id() const = 0;

    /**
     * @brief Performs forced alignment of input text tokens against audio PCM samples.
     * @param pcm_samples 16kHz mono float PCM sample buffer.
     * @param sample_rate Sample rate in Hz (e.g. 16000).
     * @param tokens Sequence of unaligned text tokens or reference words.
     * @return std::vector<AlignedWordToken> Time-locked aligned word tokens with millisecond start/end timestamps.
     */
    virtual std::vector<AlignedWordToken> align_tokens(
        const std::vector<float>& pcm_samples,
        int sample_rate,
        const std::vector<std::string>& tokens
    ) = 0;
};

#endif // I_CTC_ALIGNER_HPP
