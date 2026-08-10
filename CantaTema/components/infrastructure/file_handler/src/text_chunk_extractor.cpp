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
                size_t next_idx = end + 1;
                while (next_idx < len && (full_text[next_idx] == '\r' || full_text[next_idx] == ' ' || full_text[next_idx] == '\t')) {
                    next_idx++;
                }
                // Break on double newline (\n\n) or line break before an uppercase letter
                if (next_idx < len && (full_text[next_idx] == '\n' || std::isupper(static_cast<unsigned char>(full_text[next_idx])))) {
                    end = next_idx;
                    found_delimiter = true;
                    break;
                }
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
    unsigned int min_words = config.get_coverage_min_chunk_word_count();

    unsigned int chunk_id_counter = 1;
    unsigned int sentence_idx = 0;

    for (const auto& range : sentence_ranges) {
        size_t s_start = range.first;
        size_t s_end = range.second;

        std::string raw_chunk_text = full_text.substr(s_start, s_end - s_start);

        // Replace internal single newlines with spaces for clean continuous prose
        std::string clean_chunk_text;
        clean_chunk_text.reserve(raw_chunk_text.length());
        for (size_t i = 0; i < raw_chunk_text.length(); ++i) {
            if (raw_chunk_text[i] == '\n' || raw_chunk_text[i] == '\r') {
                if (clean_chunk_text.empty() || clean_chunk_text.back() != ' ') {
                    clean_chunk_text += ' ';
                }
            } else {
                clean_chunk_text += raw_chunk_text[i];
            }
        }

        size_t word_count = 0;
        {
            std::stringstream ss(clean_chunk_text);
            std::string w;
            while (ss >> w) word_count++;
        }

        // Helper lambdas for document artifact cleaning
        auto is_page_header_artifact = [](const std::string& str) -> bool {
            std::string s = str;
            std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
            return (s.find("página ") != std::string::npos || s.find("pagina ") != std::string::npos || s.find("pág. ") != std::string::npos)
                   && s.find(" de ") != std::string::npos;
        };

        auto is_list_label = [](const std::string& str) -> bool {
            std::string s;
            for (char c : str) {
                if (!std::isspace(static_cast<unsigned char>(c))) s += c;
            }
            if (s.empty() || s.length() > 6) return false;
            // Matches "1.", "2.", "3.", "1º.", "2º.", "a)", "b)", "1º", "2º", etc.
            bool all_label = true;
            for (char c : s) {
                if (!std::isalnum(static_cast<unsigned char>(c)) && c != '.' && c != ')' && c != '(' && c != '-' && static_cast<unsigned char>(c) != 0xba && static_cast<unsigned char>(c) != 0xaa) {
                    all_label = false;
                    break;
                }
            }
            return all_label;
        };

        if (is_page_header_artifact(clean_chunk_text)) {
            continue; // Skip PDF header/footer artifacts
        }

        // Split number rejoining (e.g. previous chunk ends with "60." or "180." and current chunk starts with "000")
        if (!out_chunks.empty() && !clean_chunk_text.empty()) {
            std::string& prev_text = out_chunks.back().text;
            if (prev_text.length() >= 3 && std::isdigit(static_cast<unsigned char>(prev_text[prev_text.length() - 2])) && prev_text.back() == '.') {
                if (clean_chunk_text.rfind("000", 0) == 0) {
                    // Rejoin "60." + "000 euros" -> "60.000 euros"
                    prev_text.pop_back(); // remove period
                    prev_text += "." + clean_chunk_text;
                    out_chunks.back().contextual_text = prev_text;
                    continue;
                }
            }
        }

        // Micro-chunk merging: merge small incomplete chunks (< min_words without sentence-ending punctuation or list labels)
        bool has_sentence_punct = false;
        if (!clean_chunk_text.empty()) {
            char last_c = clean_chunk_text.back();
            if (last_c == '.' || last_c == '?' || last_c == '!') {
                has_sentence_punct = true;
            }
        }

        if ((is_list_label(clean_chunk_text) || (word_count < min_words && !has_sentence_punct)) && !out_chunks.empty()) {
            out_chunks.back().text += " " + clean_chunk_text;
            out_chunks.back().contextual_text = out_chunks.back().text;
            continue;
        }

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
        chunk.text = clean_chunk_text;
        chunk.contextual_text = chunk.text; // default to text
        chunk.sentence_index = sentence_idx++;
        chunk.importance_weight = importance_weight;
        chunk.is_bold = is_bold;
        chunk.is_italic = is_italic;
        chunk.is_underlined = false; // default false since underlines aren't in TextSpan
        chunk.has_bg_color = has_bg_color;

        out_chunks.push_back(chunk);
    }

    // Post-processing: For short headings (<= 10 words or bold), expand contextual_text with subsequent chunk text
    for (size_t i = 0; i < out_chunks.size(); ++i) {
        size_t word_count = 0;
        std::stringstream ss(out_chunks[i].text);
        std::string word;
        while (ss >> word) word_count++;

        if ((word_count <= 10 || out_chunks[i].is_bold) && (i + 1 < out_chunks.size())) {
            out_chunks[i].contextual_text = out_chunks[i].text + " " + out_chunks[i + 1].text;
        }
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
