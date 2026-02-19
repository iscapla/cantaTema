
#include "file_handler/extension_type_pdf.hpp"
#include <set>
#include <cmath>
#include <algorithm>

namespace {
    int rgb_to_hex(float r, float g, float b) {
        int ir = std::clamp(static_cast<int>(r * 255.0f + 0.5f), 0, 255);
        int ig = std::clamp(static_cast<int>(g * 255.0f + 0.5f), 0, 255);
        int ib = std::clamp(static_cast<int>(b * 255.0f + 0.5f), 0, 255);
        return (ir << 16) | (ig << 8) | ib;
    }

    int cmyk_to_hex(float c, float m, float y, float k) {
        float r = (1.0f - c) * (1.0f - k);
        float g = (1.0f - m) * (1.0f - k);
        float b = (1.0f - y) * (1.0f - k);
        return rgb_to_hex(r, g, b);
    }

    struct HighlightInfo {
        fz_rect rect;
        int color;
    };

    struct HighlightDevice {
        fz_device super;
        std::vector<HighlightInfo> *highlights;
    };

    void capture_fill_path(fz_context *ctx, fz_device *dev, const fz_path *path, int even_odd, fz_matrix ctm, fz_colorspace *colorspace, const float *color, float alpha, fz_color_params color_params) {
        auto *custom_dev = reinterpret_cast<HighlightDevice*>(dev);
        auto *highlights = custom_dev->highlights;
        
        int hex = 0xFFFFFF;
        int n = colorspace ? fz_colorspace_n(ctx, colorspace) : 0;

        if (n == 3) {
            hex = rgb_to_hex(color[0], color[1], color[2]);
        } else if (n == 4) {
            hex = cmyk_to_hex(color[0], color[1], color[2], color[3]);
        } else if (n == 1) {
            hex = rgb_to_hex(color[0], color[0], color[0]);
        } else {
            return; 
        }

        if (hex == 0xFFFFFF || hex == 0x000000) return;

        fz_rect rect = fz_bound_path(ctx, path, nullptr, ctm);
        highlights->push_back({rect, hex});
    }
}

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
        pdf_document *pdf_doc = pdf_document_from_fz_document(m_ctx, m_doc);
        for (int page_num = 0; page_num < m_page_count; ++page_num) {
            pdf_page *page = pdf_load_page(m_ctx, pdf_doc, page_num);
            if (!page) continue;

            // 1. Load Highlights (Annotations)
            std::vector<HighlightInfo> highlights;
            
            pdf_annot *annot = pdf_first_annot(m_ctx, page);
            while (annot) {
                enum pdf_annot_type type = pdf_annot_type(m_ctx, annot);
                if (type == PDF_ANNOT_HIGHLIGHT) {
                    float color[4] = {1, 1, 0, 1}; // Default yellow
                    int n = 0;
                    pdf_annot_color(m_ctx, annot, &n, color);
                    
                    int hex_color = 0xFFFF00;
                    if (n == 1) hex_color = rgb_to_hex(color[0], color[0], color[0]);
                    else if (n == 3) hex_color = rgb_to_hex(color[0], color[1], color[2]);
                    else if (n == 4) hex_color = cmyk_to_hex(color[0], color[1], color[2], color[3]);
                    else if (n > 0) hex_color = rgb_to_hex(color[0], color[1], color[2]);

                    highlights.push_back({pdf_bound_annot(m_ctx, annot), hex_color});
                }
                annot = pdf_next_annot(m_ctx, annot);
            }

            // 2. Load Graphic Highlights (Background rectangles)
            HighlightDevice *hl_dev = (HighlightDevice*)fz_new_device_of_size(m_ctx, sizeof(HighlightDevice));
            hl_dev->highlights = &highlights;
            hl_dev->super.fill_path = capture_fill_path;
            
            pdf_run_page(m_ctx, page, (fz_device*)hl_dev, fz_identity, nullptr);
            fz_close_device(m_ctx, (fz_device*)hl_dev);
            fz_drop_device(m_ctx, (fz_device*)hl_dev);

            // 3. Load Text
            fz_stext_page *text_page = fz_new_stext_page(m_ctx, pdf_bound_page(m_ctx, page, FZ_CROP_BOX));
            fz_device *dev = fz_new_stext_device(m_ctx, text_page, nullptr);
            pdf_run_page(m_ctx, page, dev, fz_identity, nullptr);
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
            pdf_drop_page(m_ctx, page);
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
