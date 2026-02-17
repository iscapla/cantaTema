
#include "file_handler/text_handler.hpp"
#include "configuration/configuration_system.hpp"

#include "file_handler/extension_type_pdf.hpp"
#include "file_handler/extension_type_txt.hpp"

TextFileHandler::TextFileHandler(const std::string &file_path) : FileHandler(file_path, ConfigurationSystem::getInstance().get_user_default_max_text_file_size_in_mb() * 1024 * 1024)
{
    ExtensionType type = get_extension_type(file_path);

    switch (type)
    {
    case ExtensionType::TXT:
        m_pExtensionType = std::make_unique<ExtensionTypeTXT>(*this);
        break;
    case ExtensionType::PDF:
        m_pExtensionType = std::make_unique<ExtensionTypePDF>(*this);
        break;
    default:
        throw std::runtime_error("Unsupported file extension");
        break;
    }
}

TextFileHandler::~TextFileHandler(void){
    m_pExtensionType.reset();
}

TextFileHandler::ExtensionType TextFileHandler::get_extension_type(const std::string &path)
{
    if (path.empty())
        return ExtensionType::UNKNOWN;
    size_t last_dot = path.find_last_of('.');
    if (last_dot == std::string::npos)
        return ExtensionType::UNKNOWN;

    std::string ext = path.substr(last_dot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c)
                   { return std::tolower(c); });

    if (ext == "txt")
        return ExtensionType::TXT;
    if (ext == "pdf")
        return ExtensionType::PDF;
    return ExtensionType::UNKNOWN;
}

void TextFileHandler::parse() { return m_pExtensionType->parse(); }

int TextFileHandler::get_number_of_pages() const { return m_pExtensionType->get_number_of_pages(); }

std::string TextFileHandler::extract_text_content() const { return m_pExtensionType->extract_text_content(); }

std::vector<IExtensionType::TextSpan> TextFileHandler::extract_rich_text() const { return m_pExtensionType->extract_rich_text(); }

std::vector<std::string> TextFileHandler::find_bold() const { return m_pExtensionType->find_bold(); }

std::vector<std::string> TextFileHandler::find_italic() const { return m_pExtensionType->find_italic(); }

std::vector<std::string> TextFileHandler::find_highlight(int color_hex) const { return m_pExtensionType->find_highlight(color_hex); }

std::vector<float> TextFileHandler::get_font_sizes() const { return m_pExtensionType->get_font_sizes(); }

std::vector<int> TextFileHandler::get_highlighted_colors() const { return m_pExtensionType->get_highlighted_colors(); }
