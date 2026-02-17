
#include "file_handler/extension_type_txt.hpp"
#include <fstream>
#include <sstream>

ExtensionTypeTXT::ExtensionTypeTXT(const FileHandler &handler)
    : FileHandler(handler), m_parsed(false)
{
}

ExtensionTypeTXT::~ExtensionTypeTXT()
{
}

void ExtensionTypeTXT::parse()
{
    if (m_parsed) {
        return;
    }

    if (!is_file_path_valid()) {
        logger->error("Invalid file path: {}", get_file_path().string());
        return;
    }

    std::ifstream file(get_file_path());
    if (!file.is_open()) {
        logger->error("Failed to open file: {}", get_file_path().string());
        return;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    m_content = buffer.str();
    m_parsed = true;
}

int ExtensionTypeTXT::get_number_of_pages() const
{
    // Plain text is considered 1 page if it has content, otherwise 0.
    return m_content.empty() ? 0 : 1;
}

std::string ExtensionTypeTXT::extract_text_content() const
{
    if (!m_parsed) {
        logger->warn("Text content requested but file not parsed yet.");
    }
    return m_content;
}

std::vector<IExtensionType::TextSpan> ExtensionTypeTXT::extract_rich_text() const
{
    std::vector<TextSpan> spans;
    if (!m_content.empty()) {
        TextSpan span;
        span.text = m_content;
        // Default values for bold, italic, highlight etc. are already false/0
        spans.push_back(span);
    }
    return spans;
}

std::vector<std::string> ExtensionTypeTXT::find_bold() const
{
    return {};
}

std::vector<std::string> ExtensionTypeTXT::find_italic() const
{
    return {};
}

std::vector<std::string> ExtensionTypeTXT::find_highlight(int color_hex) const
{
    return {};
}

std::vector<float> ExtensionTypeTXT::get_font_sizes() const
{
    return {};
}

std::vector<int> ExtensionTypeTXT::get_highlighted_colors() const
{
    return {};
}
