
#include "file_handler/text_handler.hpp"
#include "configuration/configuration_system.hpp"

TextFileHandler::TextFileHandler(const std::string &file_path)
    : FileHandler(file_path, ConfigurationSystem::getInstance().get_user_default_max_text_file_size_in_mb() * 1024 * 1024),
      m_ctx(nullptr), m_doc(nullptr), m_page_count(0) {
}

TextFileHandler::~TextFileHandler(void){
    if (m_doc) {
        fz_drop_document(m_ctx, m_doc);
    }
    if (m_ctx) {
        fz_drop_context(m_ctx);
    }
}

void TextFileHandler::parse() {
    if (m_ctx) return; // Already parsed
    
    // Check if the file path is valid
    if (!is_file_path_valid()) {
        logger->error("Invalid file path: {}", get_file_path().string());
        return;
    }

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

int TextFileHandler::get_number_of_pages() const {
    return m_page_count;
}

std::string TextFileHandler::extract_text_content() const {
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
