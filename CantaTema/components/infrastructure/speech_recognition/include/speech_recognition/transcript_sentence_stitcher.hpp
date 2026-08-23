/**
 * @file transcript_sentence_stitcher.hpp
 * @brief Heuristic Whisper transcript sentence reconstruction and pause/punctuation stitching.
 */

#ifndef TRANSCRIPT_SENTENCE_STITCHER_HPP
#define TRANSCRIPT_SENTENCE_STITCHER_HPP

#include <vector>
#include <string>
#include <cstdint>
#include "speech_recognition/i_speech_recognition.hpp"

/**
 * @struct StitchOptions
 * @brief Configuration parameters for heuristic transcript sentence reconstruction.
 */
struct StitchOptions {
    uint64_t max_pause_gap_ms{1500};             ///< Maximum silence pause allowed between connected clauses (ms).
    uint64_t max_breath_pause_with_dot_ms{800};  ///< Maximum pause gap if segment ends with period but next starts lowercase (ms).
    size_t max_merged_words{55};                 ///< Maximum word count ceiling for a single stitched sentence.
    bool normalize_whitespace{true};             ///< Whether to collapse internal whitespace.
};

/**
 * @class TranscriptSentenceStitcher
 * @brief Reconstructs full grammatical sentences from fragmented acoustic Whisper segments.
 */
class TranscriptSentenceStitcher {
public:
    /**
     * @brief Determines whether segment 'b' is a continuation of segment 'a'.
     * 
     * @param a Preceding transcript segment.
     * @param b Succeeding transcript segment.
     * @param current_word_count Current accumulated word count of segment 'a'.
     * @param options Stitching threshold options.
     * @return true if 'b' should be merged into 'a', false otherwise.
     */
    static bool is_continuation(
        const TranscriptSegment& a,
        const TranscriptSegment& b,
        size_t current_word_count,
        const StitchOptions& options = {}
    );

    /**
     * @brief Stitches a sequence of raw acoustic transcript segments into complete sentence chunks.
     * 
     * @param raw_segments Raw transcript segments from Whisper speech recognition.
     * @param options Stitching heuristic thresholds.
     * @return std::vector<TranscriptSegment> Stitched sentence segments.
     */
    static std::vector<TranscriptSegment> stitch_segments(
        const std::vector<TranscriptSegment>& raw_segments,
        const StitchOptions& options = {}
    );

    /**
     * @brief Counts words in a string.
     */
    static size_t count_words(const std::string& text);
};

#endif // TRANSCRIPT_SENTENCE_STITCHER_HPP
