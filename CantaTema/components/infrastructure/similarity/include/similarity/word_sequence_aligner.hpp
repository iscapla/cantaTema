/**
 * @file word_sequence_aligner.hpp
 * @brief Word-level sequence alignment engine (LCS / Myers Diff) for fine-grained omission detection.
 */

#ifndef WORD_SEQUENCE_ALIGNER_HPP
#define WORD_SEQUENCE_ALIGNER_HPP

#include <string>
#include <vector>
#include <cstddef>

/**
 * @enum WordDiffStatus
 * @brief Categorization status of a word in a reference text chunk.
 */
enum class WordDiffStatus {
    MATCHED = 0,     /// Word spoken correctly by user
    OMITTED,         /// Word present in reference but missing in spoken audio
    SUBSTITUTED      /// Word replaced or misquoted
};

/**
 * @struct WordDiffToken
 * @brief Detailed status token for an individual reference word.
 */
struct WordDiffToken {
    std::string original_word;
    std::string normalized_word;
    WordDiffStatus status = WordDiffStatus::MATCHED;
    size_t index = 0;
    float weight = 1.0f;           ///< Linguistic importance weight (4.0x legal citations, 3.0x numbers, 0.2x stopwords)
    bool is_legal_citation = false; ///< True if token is part of an article/law/enumerator reference
    bool is_numeric = false;       ///< True if token contains numeric data, dates, or percentages
    bool is_stopword = false;      ///< True if token is a minor grammatical article or preposition
};

/**
 * @struct WordAlignmentResult
 * @brief Complete sequence alignment breakdown for a reference text chunk vs spoken transcript.
 */
struct WordAlignmentResult {
    std::vector<WordDiffToken> reference_words;
    size_t matched_word_count = 0;
    size_t omitted_word_count = 0;
    size_t substituted_word_count = 0;
    size_t total_reference_words = 0;
    float word_recall_score = 0.0f;       ///< Unweighted word recall: matched_word_count / total_reference_words
    float total_reference_weight = 0.0f;  ///< Sum of all token weights in reference chunk
    float matched_reference_weight = 0.0f;///< Sum of weights of matched tokens
    float weighted_recall_score = 0.0f;   ///< Importance-weighted recall: matched_reference_weight / total_reference_weight
    bool has_missing_legal_citation = false; ///< True if an article/law/enumerator citation was omitted in audio
};

/**
 * @class WordSequenceAligner
 * @brief Utility for performing tokenization, normalization, and sequence alignment (LCS) between reference text and transcript text.
 */
class WordSequenceAligner {
public:
    /**
     * @brief Normalizes a single word by converting to lowercase, removing punctuation, and stripping accents.
     */
    static std::string normalize_word(const std::string& input);

    /**
     * @brief Tokenizes input string into whitespace-separated words.
     */
    static std::vector<std::string> tokenize(const std::string& text);

    /**
     * @brief Aligns a reference text string against a candidate transcript string.
     * 
     * @param reference_text The reference chunk from PDF.
     * @param transcript_text The spoken transcript segment from Whisper STT.
     * @return WordAlignmentResult Detailed token breakdown and word recall metrics.
     */
    static WordAlignmentResult align(const std::string& reference_text, const std::string& transcript_text);
};

#endif // WORD_SEQUENCE_ALIGNER_HPP
