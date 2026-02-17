#ifndef TEXT_FILE_HANDLER_HPP
#define TEXT_FILE_HANDLER_HPP

#include <vector>
#include <mupdf/fitz.h>
#include <string>

#include "file_handler/file_handler.hpp"


class TextFileHandler : public FileHandler{

public:
    TextFileHandler(const std::string &file_path);
    ~TextFileHandler(void);
    
    /**
     * @brief Parses the document (opens file, counts pages).
     */
    void parse();

    /**
     * @brief Returns the number of pages in the document.
     * @return int Number of pages.
     */
    int get_number_of_pages() const;

    /**
     * @brief Extracts text content from the file.
     * 
     * @return std::string The extracted text.
     */
    std::string extract_text_content() const;
    
private:
    fz_context *m_ctx;
    fz_document *m_doc;
    int m_page_count;
};

#endif // TEXT_FILE_HANDLER_HPP