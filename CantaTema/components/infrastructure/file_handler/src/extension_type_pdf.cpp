
#include "file_handler/extension_type_pdf.hpp"
#include <set>
#include <cmath>

ExtensionTypePDF::ExtensionTypePDF(const FileHandler &handler)
    : FileHandler(handler), m_ctx(nullptr), m_doc(nullptr), m_page_count(0), m_spans_cached(false) {
}

ExtensionTypePDF::~ExtensionTypePDF(void){
    if (m_doc) {
        fz_drop_document(m_ctx, m_doc);
    }
    if (m_ctx) {
        fz_drop_context(m_ctx);
    }
}

void ExtensionTypePDF::parse() {
    if (m_ctx) return; // Already parsed
    
    // Check if the file path is valid
    if (!is_file_path_valid()) {
        logger->error("Invalid file path: {}", get_file_path().string());
        return;
    }

    m_spans_cached = false;
    m_cached_spans.clear();

    // Initialize MuPDF context
    m_ctx = fz_new_context(nullptr, nullptr, FZ_STORE_DEFAULT);
    if (!m_ctx) {
        logger->error("Failed to create MuPDF context.");
        return;
    }

    fz_try(m_ctx) {
        fz_register_document_handlers(m_ctx);

        // Open the PDF document
        m_doc = fz_open_document(m_ctx, get_file_path().string().c_str());
        if (!m_doc) {
            logger->error("Failed to open document: {}", get_file_path().string());
        } else {
            m_page_count = fz_count_pages(m_ctx, m_doc);
        }
    } fz_catch(m_ctx) {
        logger->error("MuPDF error in parse: {}", fz_caught_message(m_ctx));
    }
}

int ExtensionTypePDF::get_number_of_pages() const {
    return m_page_count;
}

std::string ExtensionTypePDF::extract_text_content() const {
    std::string text_content;

    if (!m_ctx || !m_doc) {
        logger->error("Document not parsed. Call parse() first.");
        return "";
    }

    fz_try(m_ctx) {

        // Iterate through each page
        for (int page_num = 0; page_num < m_page_count; ++page_num) {
            fz_page *page = fz_load_page(m_ctx, m_doc, page_num);
            if (!page) {
                logger->error("Failed to load page {}.", page_num + 1);
                continue;
            }

            fz_stext_page *text_page = fz_new_stext_page(m_ctx, fz_bound_page(m_ctx, page));
            fz_device *dev = fz_new_stext_device(m_ctx, text_page, nullptr);
            fz_run_page(m_ctx, page, dev, fz_identity, nullptr);
            fz_close_device(m_ctx, dev);
            fz_drop_device(m_ctx, dev);

            // Extract text from the text page
            for (fz_stext_block *block = text_page->first_block; block; block = block->next) {
                if (block->type == FZ_STEXT_BLOCK_TEXT) {
                    for (fz_stext_line *line = block->u.t.first_line; line; line = line->next) {
                        for (fz_stext_char *ch = line->first_char; ch; ch = ch->next) {
                            char buf[8];
                            int n = fz_runetochar(buf, ch->c);
                            text_content.append(buf, n);
                        }
                        text_content += "\n";
                    }
                }
            }
            fz_drop_stext_page(m_ctx, text_page);
            fz_drop_page(m_ctx, page);
        }
    } fz_catch(m_ctx) {
        logger->error("MuPDF error: {}", fz_caught_message(m_ctx));
    }

    return text_content;
}

std::vector<ExtensionTypePDF::TextSpan> ExtensionTypePDF::extract_rich_text() const {
    if (m_spans_cached) {
        return m_cached_spans;
    }

    if (!m_ctx || !m_doc) {
        logger->error("Document not parsed. Call parse() first.");
        return {};
    }

    m_cached_spans.clear();

    fz_try(m_ctx) {
        for (int page_num = 0; page_num < m_page_count; ++page_num) {
            fz_page *page = fz_load_page(m_ctx, m_doc, page_num);
            if (!page) continue;

            // 1. Load Highlights (Annotations)
            struct HighlightInfo {
                fz_rect rect;
                int color;
            };
            std::vector<HighlightInfo> highlights;
            
            pdf_page *pdf_pg = pdf_page_from_fz_page(m_ctx, page);
            if (pdf_pg) {
                pdf_annot *annot = pdf_first_annot(m_ctx, pdf_pg);
                while (annot) {
                    if (pdf_annot_type(m_ctx, annot) == PDF_ANNOT_HIGHLIGHT) {
                        float color[4] = {1, 1, 0, 1}; // Default yellow
                        int n = 0;
                        pdf_annot_color(m_ctx, annot, &n, color);
                        highlights.push_back({pdf_bound_annot(m_ctx, annot), rgb_to_hex(color[0], color[1], color[2])});
                    }
                    annot = pdf_next_annot(m_ctx, annot);
                }
            }

            // 2. Load Text
            fz_stext_page *text_page = fz_new_stext_page(m_ctx, fz_bound_page(m_ctx, page));
            fz_device *dev = fz_new_stext_device(m_ctx, text_page, nullptr);
            fz_run_page(m_ctx, page, dev, fz_identity, nullptr);
            fz_close_device(m_ctx, dev);
            fz_drop_device(m_ctx, dev);

            TextSpan current_span;
            bool first_char = true;

            for (fz_stext_block *block = text_page->first_block; block; block = block->next) {
                if (block->type != FZ_STEXT_BLOCK_TEXT) continue;

                for (fz_stext_line *line = block->u.t.first_line; line; line = line->next) {
                    for (fz_stext_char *ch = line->first_char; ch; ch = ch->next) {
                        
                        bool is_bold = fz_font_is_bold(m_ctx, ch->font) != 0;
                        bool is_italic = fz_font_is_italic(m_ctx, ch->font) != 0;
                        float font_size = ch->size;
                        int text_color = ch->argb & 0xFFFFFF; // Extract RGB from ARGB

                        bool is_highlighted = false;
                        int highlight_color = 0;

                        fz_rect char_box = fz_rect_from_quad(ch->quad);
                        for (const auto& h : highlights) {
                            fz_rect intersection = fz_intersect_rect(char_box, h.rect);
                            if (!fz_is_empty_rect(intersection)) {
                                is_highlighted = true;
                                highlight_color = h.color;
                                break;
                            }
                        }

                        if (first_char) {
                            current_span.is_bold = is_bold;
                            current_span.is_italic = is_italic;
                            current_span.font_size = font_size;
                            current_span.text_color = text_color;
                            current_span.is_highlighted = is_highlighted;
                            current_span.highlight_color = highlight_color;
                            first_char = false;
                        } else {
                            // Check if attributes changed
                            if (is_bold != current_span.is_bold ||
                                is_italic != current_span.is_italic ||
                                std::abs(font_size - current_span.font_size) > 0.1f ||
                                text_color != current_span.text_color ||
                                is_highlighted != current_span.is_highlighted ||
                                highlight_color != current_span.highlight_color) {
                                
                                m_cached_spans.push_back(current_span);
                                current_span = TextSpan();
                                current_span.is_bold = is_bold;
                                current_span.is_italic = is_italic;
                                current_span.font_size = font_size;
                                current_span.text_color = text_color;
                                current_span.is_highlighted = is_highlighted;
                                current_span.highlight_color = highlight_color;
                            }
                        }

                        char buf[8];
                        int n = fz_runetochar(buf, ch->c);
                        current_span.text.append(buf, n);
                    }
                    current_span.text += "\n";
                }
            }
            if (!current_span.text.empty()) {
                m_cached_spans.push_back(current_span);
            }

            fz_drop_stext_page(m_ctx, text_page);
            fz_drop_page(m_ctx, page);
        }
        m_spans_cached = true;
    } fz_catch(m_ctx) {
        logger->error("MuPDF error: {}", fz_caught_message(m_ctx));
    }

    return m_cached_spans;
}

std::vector<std::string> ExtensionTypePDF::find_bold() const {
    std::vector<std::string> results;
    const auto& spans = extract_rich_text();
    for (const auto& span : spans) {
        if (span.is_bold) {
            results.push_back(span.text);
        }
    }
    return results;
}

std::vector<std::string> ExtensionTypePDF::find_italic() const {
    std::vector<std::string> results;
    const auto& spans = extract_rich_text();
    for (const auto& span : spans) {
        if (span.is_italic) {
            results.push_back(span.text);
        }
    }
    return results;
}

std::vector<std::string> ExtensionTypePDF::find_highlight(int color_hex) const {
    std::vector<std::string> results;
    const auto& spans = extract_rich_text();
    for (const auto& span : spans) {
        if (span.is_highlighted && span.highlight_color == color_hex) {
            results.push_back(span.text);
        }
    }
    return results;
}

std::vector<float> ExtensionTypePDF::get_font_sizes() const {
    std::set<float> sizes;
    const auto& spans = extract_rich_text();
    for (const auto& span : spans) {
        sizes.insert(span.font_size);
    }
    return std::vector<float>(sizes.begin(), sizes.end());
}

std::vector<int> ExtensionTypePDF::get_highlighted_colors() const {
    std::set<int> colors;
    const auto& spans = extract_rich_text();
    for (const auto& span : spans) {
        if (span.is_highlighted) {
            colors.insert(span.highlight_color);
        }
    }
    return std::vector<int>(colors.begin(), colors.end());
}
