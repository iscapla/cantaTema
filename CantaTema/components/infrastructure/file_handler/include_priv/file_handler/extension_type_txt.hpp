#ifndef EXTENSION_TYPE_TXT_HPP
#define EXTENSION_TYPE_TXT_HPP

#include <string>
#include <vector>

#include "file_handler/i_extension_type.hpp"

class ExtensionTypeTXT : public IExtensionType{

public:

    explicit ExtensionTypeTXT(const FileHandler &handler);
    ~ExtensionTypeTXT();

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
    std::string m_content;
    bool m_parsed = false;

};

#endif // EXTENSION_TYPE_TXT_HPP