#ifndef TEXT_FILE_HANDLER_HPP
#define TEXT_FILE_HANDLER_HPP

#include <memory>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>

#include "file_handler/i_extension_type.hpp"


class TextFileHandler : public IExtensionType{

public:

    enum class ExtensionType {
        TXT,
        PDF,
        UNKNOWN
    };

    TextFileHandler(const std::string &file_path);
    ~TextFileHandler(void);
    
    /**
     * @brief Parses the document (opens file, counts pages).
     */
    void parse() override;

    /**
     * @brief Returns the number of pages in the document.
     * @return int Number of pages.
     */
    int get_number_of_pages() const override;

    /**
     * @brief Extracts text content from the file.
     * 
     * @return std::string The extracted text.
     */
    std::string extract_text_content() const override;

    /**
     * @brief Extracts text content with rich formatting attributes.
     * 
     * @return std::vector<TextSpan> A vector of text spans with attributes.
     */
    std::vector<IExtensionType::TextSpan> extract_rich_text() const override;

    /**
     * @brief Finds all text that is bold.
     * 
     * @return std::vector<std::string> List of bold text segments.
     */
    std::vector<std::string> find_bold() const override;

    /**
     * @brief Finds all text that is italic.
     * 
     * @return std::vector<std::string> List of italic text segments.
     */
    std::vector<std::string> find_italic() const override;

    /**
     * @brief Finds all text highlighted with a specific color.
     * 
     * @param color_hex The highlight color in hex (0xRRGGBB).
     * @return std::vector<std::string> List of highlighted text segments.
     */
    std::vector<std::string> find_highlight(int color_hex) const override;

    /**
     * @brief Returns a list of all unique font sizes found in the document.
     * 
     * @return std::vector<float> List of font sizes.
     */
    std::vector<float> get_font_sizes() const override;

    /**
     * @brief Returns a list of all unique highlight colors found in the document.
     * 
     * @return std::vector<int> List of highlight colors (0xRRGGBB).
     */
    std::vector<int> get_highlighted_colors() const override;

private:
    std::unique_ptr<IExtensionType> m_pExtensionType;

    ExtensionType get_extension_type(const std::string &path);

};

#endif // TEXT_FILE_HANDLER_HPP