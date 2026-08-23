/**
 * @file transcript_sentence_stitcher.cpp
 * @brief Implementation of heuristic Whisper transcript sentence reconstruction.
 */

#include "speech_recognition/transcript_sentence_stitcher.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

namespace {

std::string get_first_word(const std::string& text) {
    std::stringstream ss(text);
    std::string w;
    if (ss >> w) {
        std::string clean;
        for (char c : w) {
            if (std::isalnum(static_cast<unsigned char>(c))) {
                clean += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
        }
        return clean;
    }
    return "";
}

bool starts_with_subordinate_marker(const std::string& text) {
    std::string first_w = get_first_word(text);
    return (first_w == "desde" || first_w == "hasta" || first_w == "mientras" || first_w == "aunque" || first_w == "si");
}

bool starts_with_relative_conjunction(const std::string& text) {
    std::string first_w = get_first_word(text);
    return (first_w == "y" || first_w == "e" || first_w == "o" || first_w == "u" || first_w == "que" || first_w == "donde" || first_w == "cuando");
}

} // namespace

size_t TranscriptSentenceStitcher::count_words(const std::string& text) {
    size_t count = 0;
    std::stringstream ss(text);
    std::string w;
    while (ss >> w) {
        count++;
    }
    return count;
}

bool TranscriptSentenceStitcher::is_continuation(
    const TranscriptSegment& a,
    const TranscriptSegment& b,
    size_t current_word_count,
    const StitchOptions& options
) {
    if (current_word_count >= options.max_merged_words) {
        return false;
    }

    if (a.text.empty() || b.text.empty()) {
        return false;
    }

    uint64_t gap_ms = (b.start_time_ms > a.end_time_ms) ? (b.start_time_ms - a.end_time_ms) : 0;

    // Find last non-space character of segment 'a'
    size_t a_end = a.text.length();
    while (a_end > 0 && std::isspace(static_cast<unsigned char>(a.text[a_end - 1]))) {
        a_end--;
    }
    if (a_end == 0) return false;
    char last_c = a.text[a_end - 1];

    // Find first non-space character of segment 'b'
    size_t b_start = 0;
    while (b_start < b.text.length() && std::isspace(static_cast<unsigned char>(b.text[b_start]))) {
        b_start++;
    }
    if (b_start >= b.text.length()) return false;
    unsigned char first_c = static_cast<unsigned char>(b.text[b_start]);

    // Check if segment 'b' starts with a lowercase letter (including UTF-8 Spanish accents)
    bool b_starts_lowercase = false;
    if (std::islower(first_c)) {
        b_starts_lowercase = true;
    } else if (first_c == 0xC3 && b_start + 1 < b.text.length()) {
        // UTF-8 Spanish lowercase letters: á, é, í, ó, ú, ñ
        unsigned char c2 = static_cast<unsigned char>(b.text[b_start + 1]);
        if (c2 == 0xA1 || c2 == 0xA9 || c2 == 0xAD || c2 == 0xB3 || c2 == 0xBA || c2 == 0xB1) {
            b_starts_lowercase = true;
        }
    }

    // Case 1: Explicit Continuation Punctuation (comma, semicolon, colon, dash)
    if (last_c == ',' || last_c == ';' || last_c == ':' || last_c == '-') {
        return (gap_ms <= options.max_pause_gap_ms);
    }

    // Case 2: No terminal punctuation (letter, number, or closing delimiter without period)
    if (last_c != '.' && last_c != '?' && last_c != '!') {
        return (gap_ms <= options.max_pause_gap_ms);
    }

    // Case 3: Ends with period - keep sentences SPLIT as much as possible!
    // Only merge if segment 'a' was an incomplete subordinate clause or segment 'b' is a lowercase continuation or conjunction.
    if (last_c == '.') {
        if (gap_ms <= options.max_breath_pause_with_dot_ms) {
            if (b_starts_lowercase || starts_with_relative_conjunction(b.text) || starts_with_subordinate_marker(a.text)) {
                return true;
            }
        }
    }

    return false;
}

std::vector<TranscriptSegment> TranscriptSentenceStitcher::stitch_segments(
    const std::vector<TranscriptSegment>& raw_segments,
    const StitchOptions& options
) {
    std::vector<TranscriptSegment> result;
    if (raw_segments.empty()) {
        return result;
    }

    TranscriptSegment current = raw_segments[0];
    current.source_segment_indices = {0};
    size_t current_words = count_words(current.text);
    uint64_t current_dur = (current.end_time_ms > current.start_time_ms) ? (current.end_time_ms - current.start_time_ms) : 1;
    double weighted_conf_sum = current.confidence_score * current_dur;
    double weighted_logprob_sum = current.avg_logprob * current_dur;

    for (size_t i = 1; i < raw_segments.size(); ++i) {
        const auto& next_seg = raw_segments[i];
        size_t next_words = count_words(next_seg.text);

        if (is_continuation(current, next_seg, current_words, options)) {
            // Remove trailing period if it was a false breath pause period
            if (!current.text.empty()) {
                size_t len = current.text.length();
                while (len > 0 && std::isspace(static_cast<unsigned char>(current.text[len - 1]))) {
                    len--;
                }
                if (len > 0 && current.text[len - 1] == '.') {
                    current.text.erase(len - 1, 1);
                }
            }

            // Cleanly concatenate next segment text
            if (!current.text.empty() && current.text.back() != ' ') {
                current.text += " ";
            }

            // Strip leading whitespace from next segment
            size_t n_start = 0;
            while (n_start < next_seg.text.length() && std::isspace(static_cast<unsigned char>(next_seg.text[n_start]))) {
                n_start++;
            }
            current.text += next_seg.text.substr(n_start);

            // Track merged source index
            current.source_segment_indices.push_back(i);

            // Extend timestamps
            current.end_time_ms = std::max(current.end_time_ms, next_seg.end_time_ms);
            current_words += next_words;

            uint64_t next_dur = (next_seg.end_time_ms > next_seg.start_time_ms) ? (next_seg.end_time_ms - next_seg.start_time_ms) : 1;
            weighted_conf_sum += next_seg.confidence_score * next_dur;
            weighted_logprob_sum += next_seg.avg_logprob * next_dur;
            current_dur += next_dur;

            current.confidence_score = static_cast<float>(weighted_conf_sum / current_dur);
            current.avg_logprob = static_cast<float>(weighted_logprob_sum / current_dur);
        } else {
            result.push_back(current);
            current = next_seg;
            current.source_segment_indices = {i};
            current_words = next_words;
            current_dur = (current.end_time_ms > current.start_time_ms) ? (current.end_time_ms - current.start_time_ms) : 1;
            weighted_conf_sum = current.confidence_score * current_dur;
            weighted_logprob_sum = current.avg_logprob * current_dur;
        }
    }

    result.push_back(current);
    return result;
}
