#include "file_handler/text_chunk_extractor.hpp"
#include "configuration/configuration_system.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>

rst_code_e TextChunkExtractor::extract_chunks(const TextFileHandler& handler, std::vector<DocumentChunk>& out_chunks) const {
    ConfigurationSystem& config = ConfigurationSystem::getInstance();
    
    // 1. Enforce max page limits
    unsigned int page_count = static_cast<unsigned int>(handler.get_number_of_pages());
    unsigned int max_page_limit = config.get_max_pdf_page_count();
    if (page_count > max_page_limit) {
        logger->error("File exceeds the maximum page count limit (Pages: {}, Limit: {})", page_count, max_page_limit);
        return FILE_EXCEEDS_PAGE_LIMIT;
    }

    // 2. Extract rich text spans
    std::vector<IExtensionType::TextSpan> spans = handler.extract_rich_text();
    if (spans.empty()) {
        logger->error("Rich text spans are empty. Document might be empty or unparseable.");
        return FILE_EMPTY_OR_INVALID;
    }

    // 3. Reconstruct full text and track span boundary indices
    std::string full_text = "";
    struct SpanOffset {
        size_t start;
        size_t end;
        bool is_bold;
        bool is_italic;
        bool is_highlighted;
    };
    std::vector<SpanOffset> span_offsets;

    for (const auto& span : spans) {
        size_t start_idx = full_text.length();
        full_text += span.text;
        size_t end_idx = full_text.length();
        span_offsets.push_back({start_idx, end_idx, span.is_bold, span.is_italic, span.is_highlighted});
    }

    // Check if the overall text is empty or only whitespace
    bool has_content = false;
    for (char c : full_text) {
        if (!std::isspace(static_cast<unsigned char>(c))) {
            has_content = true;
            break;
        }
    }
    if (!has_content) {
        logger->error("Reconstructed document text is empty or contains only whitespace.");
        return FILE_EMPTY_OR_INVALID;
    }

    // 4. Tokenize full text into sentence-level chunks
    std::vector<std::pair<size_t, size_t>> sentence_ranges;
    size_t start = 0;
    size_t len = full_text.length();

    while (start < len) {
        // Skip leading whitespace
        while (start < len && std::isspace(static_cast<unsigned char>(full_text[start]))) {
            start++;
        }
        if (start >= len) break;

        size_t end = start;
        bool found_delimiter = false;

        while (end < len) {
            char c = full_text[end];
            if (c == '\n') {
                end++;
                found_delimiter = true;
                break;
            }
            if (c == '.' || c == '?' || c == '!') {
                // Check if it's a period representing an abbreviation
                if (c == '.') {
                    bool is_abbr = false;
                    const std::vector<std::string> abbrs = {
                        "pag", "pags", "ej", "art", "vol", "vs", "etc", 
                        "dr", "sr", "sra", "av", "gen", "cba", "pág", "págs"
                    };
                    for (const auto& abbr : abbrs) {
                        size_t abbr_len = abbr.length();
                        if (end >= abbr_len) {
                            size_t word_start = end - abbr_len;
                            bool match = true;
                            for (size_t i = 0; i < abbr_len; ++i) {
                                if (std::tolower(static_cast<unsigned char>(full_text[word_start + i])) != std::tolower(static_cast<unsigned char>(abbr[i]))) {
                                    match = false;
                                    break;
                                }
                            }
                            if (match) {
                                // Word boundary check before abbreviation
                                if (word_start == 0 || 
                                    std::isspace(static_cast<unsigned char>(full_text[word_start - 1])) || 
                                    std::ispunct(static_cast<unsigned char>(full_text[word_start - 1]))) {
                                    is_abbr = true;
                                    break;
                                }
                            }
                        }
                    }
                    if (is_abbr) {
                        end++;
                        continue; // Skip splitting for this dot
                    }
                }

                end++;
                found_delimiter = true;
                break;
            }
            end++;
        }

        if (!found_delimiter) {
            end = len;
        }

        // Trim trailing spaces in range
        size_t actual_end = end;
        while (actual_end > start && std::isspace(static_cast<unsigned char>(full_text[actual_end - 1]))) {
            actual_end--;
        }

        if (actual_end > start) {
            sentence_ranges.push_back({start, actual_end});
        }
        start = end;
    }

    if (sentence_ranges.empty()) {
        logger->error("No valid sentence chunks could be parsed.");
        return FILE_EMPTY_OR_INVALID;
    }

    // 5. Aggregate formatting attributes and calculate weights
    double bold_mult = config.get_importance_weight_bold();
    double italic_mult = config.get_importance_weight_italic();
    double underline_mult = config.get_importance_weight_underline();
    double bg_color_mult = config.get_importance_weight_bg_color();

    unsigned int chunk_id_counter = 1;
    unsigned int sentence_idx = 0;

    for (const auto& range : sentence_ranges) {
        size_t s_start = range.first;
        size_t s_end = range.second;

        bool is_bold = false;
        bool is_italic = false;
        bool has_bg_color = false;

        // Check formatting flags for any overlapping rich text spans
        for (const auto& offset : span_offsets) {
            if (std::max(s_start, offset.start) < std::min(s_end, offset.end)) {
                if (offset.is_bold) is_bold = true;
                if (offset.is_italic) is_italic = true;
                if (offset.is_highlighted) has_bg_color = true;
            }
        }

        // Weight calculation
        double importance_weight = 1.0;
        if (is_bold) importance_weight *= bold_mult;
        if (is_italic) importance_weight *= italic_mult;
        if (has_bg_color) importance_weight *= bg_color_mult;

        DocumentChunk chunk;
        chunk.chunk_id = chunk_id_counter++;
        chunk.text = full_text.substr(s_start, s_end - s_start);
        chunk.sentence_index = sentence_idx++;
        chunk.importance_weight = importance_weight;
        chunk.is_bold = is_bold;
        chunk.is_italic = is_italic;
        chunk.is_underlined = false; // default false since underlines aren't in TextSpan
        chunk.has_bg_color = has_bg_color;

        out_chunks.push_back(chunk);
    }

    return RST_OK;
}

rst_code_e TextChunkExtractor::extract_chunks(const TextFileHandler& handler, std::vector<DocumentChunk>& out_chunks, const UserConfiguration& config) const {
    rst_code_e res = extract_chunks(handler, out_chunks);
    if (res != RST_OK) return res;

    // Apply custom importance weights from user configuration
    double bold_mult = config.reference_extraction.importance_weight_bold;
    double italic_mult = config.reference_extraction.importance_weight_italic;
    double underline_mult = config.reference_extraction.importance_weight_underline;
    double bg_color_mult = config.reference_extraction.importance_weight_bg_color;

    for (auto& chunk : out_chunks) {
        double weight = 1.0;
        if (chunk.is_bold) weight *= bold_mult;
        if (chunk.is_italic) weight *= italic_mult;
        if (chunk.is_underlined) weight *= underline_mult;
        if (chunk.has_bg_color) weight *= bg_color_mult;
        chunk.importance_weight = weight;
    }

    return RST_OK;
}
