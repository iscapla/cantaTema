#ifndef TEXT_CHUNK_EXTRACTOR_HPP
#define TEXT_CHUNK_EXTRACTOR_HPP

#include "file_handler/text_handler.hpp"
#include <string>
#include <vector>

struct DocumentChunk {
    unsigned int chunk_id;
    std::string text;
    unsigned int sentence_index;
    double importance_weight;
    bool is_bold = false;
    bool is_italic = false;
    bool is_underlined = false;
    bool has_bg_color = false;
};

class TextChunkExtractor {
public:
    TextChunkExtractor(void) = default;
    ~TextChunkExtractor(void) = default;

    /**
     * @brief Extracts sentence-level chunks with computed importance weights from the given text handler.
     * 
     * @param handler The TextFileHandler pointing to the parsed document.
     * @param out_chunks The output vector where extracted chunks will be stored.
     * @return rst_code_e RST_OK if successful, or error code (e.g. FILE_EXCEEDS_PAGE_LIMIT, FILE_EMPTY_OR_INVALID).
     */
    rst_code_e extract_chunks(const TextFileHandler& handler, std::vector<DocumentChunk>& out_chunks) const;
};

#endif // TEXT_CHUNK_EXTRACTOR_HPP
