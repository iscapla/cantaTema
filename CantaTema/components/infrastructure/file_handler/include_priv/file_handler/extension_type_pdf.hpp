#ifndef EXTENSION_TYPE_PDF_HPP
#define EXTENSION_TYPE_PDF_HPP

#include <mupdf/fitz.h>
#include <mupdf/pdf.h>
#include <string>
#include <vector>

#include "file_handler/i_extension_type.hpp"

class ExtensionTypePDF : public IExtensionType{

public:

    explicit ExtensionTypePDF(const FileHandler &handler);
    ~ExtensionTypePDF();

    void parse() override;
    int get_number_of_pages() const override;
    std::string extract_text_content() const override;
    std::vector<TextSpan> extract_rich_text() const override;
    std::vector<std::string> find_bold() const override;
    std::vector<std::string> find_italic() const override;
    std::vector<std::string> find_highlight(int color_hex) const override;
    std::vector<float> get_font_sizes() const override;
    std::vector<int> get_highlighted_colors() const override;

private:
    fz_context *m_ctx;
    fz_document *m_doc;
    int m_page_count;
    mutable std::vector<TextSpan> m_cached_spans;
    mutable bool m_spans_cached;
};

#endif // EXTENSION_TYPE_PDF_HPP