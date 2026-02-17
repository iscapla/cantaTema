#ifndef I_EXTENSION_TYPE_HPP
#define I_EXTENSION_TYPE_HPP

#include <vector>
#include <string>

#include "file_handler/file_handler.hpp"

class IExtensionType : virtual public FileHandler{

public:
    struct TextSpan {
        std::string text;
        bool is_bold = false;
        bool is_italic = false;
        float font_size = 0.0f;
        int text_color = 0;
        bool is_highlighted = false;
        int highlight_color = 0;
    };

    virtual ~IExtensionType() = default;
    
    virtual void parse() = 0;
    virtual int get_number_of_pages() const = 0;
    virtual std::string extract_text_content() const = 0;
    virtual std::vector<IExtensionType::TextSpan> extract_rich_text() const = 0;
    virtual std::vector<std::string> find_bold() const = 0;
    virtual std::vector<std::string> find_italic() const = 0;
    virtual std::vector<std::string> find_highlight(int color_hex) const = 0;
    virtual std::vector<float> get_font_sizes() const = 0;
    virtual std::vector<int> get_highlighted_colors() const = 0;
    
protected:
    int rgb_to_hex(float r, float g, float b) const {
        int ri = static_cast<int>(r * 255.0f + 0.5f);
        int gi = static_cast<int>(g * 255.0f + 0.5f);
        int bi = static_cast<int>(b * 255.0f + 0.5f);
        return (ri << 16) | (gi << 8) | bi;
    }
};

#endif // I_EXTENSION_TYPE_HPP