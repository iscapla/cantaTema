/**
 * @file lexical_keyword_extractor.hpp
 * @brief Utility class for keyword tokenization, stop-word removal, and lexical overlap computation.
 */

#ifndef LEXICAL_KEYWORD_EXTRACTOR_HPP
#define LEXICAL_KEYWORD_EXTRACTOR_HPP

#include <string>
#include <vector>
#include <unordered_set>
#include <cstddef>

/**
 * @class LexicalKeywordExtractor
 * @brief Tokenizes text and extracts meaningful non-stopword content tokens for hybrid lexical matching.
 */
class LexicalKeywordExtractor {
public:
    LexicalKeywordExtractor() = default;
    ~LexicalKeywordExtractor() = default;

    /**
     * @brief Counts the total number of words in a string.
     * 
     * @param text Input text string.
     * @return size_t Word count.
     */
    static size_t count_words(const std::string& text);

    /**
     * @brief Extracts normalized content keyword tokens (filtering punctuation and stop-words).
     * 
     * @param text Input text string.
     * @return std::unordered_set<std::string> Set of lower-case content keyword tokens.
     */
    static std::unordered_set<std::string> extract_keywords(const std::string& text);

    /**
     * @brief Computes the number of overlapping keywords between two sets.
     * 
     * @param ref_keywords Keywords from reference chunk.
     * @param trans_keywords Keywords from candidate transcript chunk.
     * @return size_t Number of matching keywords.
     */
    static size_t count_keyword_overlap(
        const std::unordered_set<std::string>& ref_keywords,
        const std::unordered_set<std::string>& trans_keywords
    );
};

#endif // LEXICAL_KEYWORD_EXTRACTOR_HPP
