#include <gtest/gtest.h>
#include <filesystem>
#include <string>
#include <vector>
#include <iostream>
#include <iomanip>

#include "file_handler/text_handler.hpp"

namespace fs = std::filesystem;

class ExtensionTypePdfTest : public ::testing::Test {
protected:
    fs::path pdf_path;

    void SetUp() override {
        pdf_path = fs::path("example_data") / "subject_es_1.pdf";
    }
};

TEST_F(ExtensionTypePdfTest, ParseAndGetNumberOfPages) {
    if (!fs::exists(pdf_path)) {
        GTEST_SKIP() << "Test file subject_es_1.pdf not found at " << pdf_path;
    }

    TextFileHandler handler(pdf_path.string());
    handler.parse();

    // Expecting 2 pages as per previous context from t_text_handler.cxx
    EXPECT_EQ(handler.get_number_of_pages(), 2);
}

TEST_F(ExtensionTypePdfTest, ExtractTextContent) {
    if (!fs::exists(pdf_path)) {
        GTEST_SKIP() << "Test file subject_es_1.pdf not found at " << pdf_path;
    }

    TextFileHandler handler(pdf_path.string());
    handler.parse();

    std::string content = handler.extract_text_content();
    EXPECT_FALSE(content.empty());
    
    // Verify content known to exist in the file
    EXPECT_NE(content.find("tecnología"), std::string::npos);
    EXPECT_NE(content.find("transformación"), std::string::npos);
}

TEST_F(ExtensionTypePdfTest, ExtractRichText) {
    if (!fs::exists(pdf_path)) {
        GTEST_SKIP() << "Test file subject_es_1.pdf not found at " << pdf_path;
    }

    TextFileHandler handler(pdf_path.string());
    handler.parse();

    auto spans = handler.extract_rich_text();
    EXPECT_FALSE(spans.empty());

    bool found_bold = false;
    bool found_italic = false;

    for (const auto& span : spans) {
        if (span.is_bold) found_bold = true;
        if (span.is_italic) found_italic = true;
    }

    EXPECT_TRUE(found_bold) << "Expected bold text in rich text extraction";
    EXPECT_TRUE(found_italic) << "Expected italic text in rich text extraction";
}

TEST_F(ExtensionTypePdfTest, FindBold) {
    if (!fs::exists(pdf_path)) {
        GTEST_SKIP() << "Test file subject_es_1.pdf not found at " << pdf_path;
    }

    TextFileHandler handler(pdf_path.string());
    handler.parse();

    auto bold_texts = handler.find_bold();
    EXPECT_FALSE(bold_texts.empty()) << "Expected bold text segments";
}

TEST_F(ExtensionTypePdfTest, FindItalic) {
    if (!fs::exists(pdf_path)) {
        GTEST_SKIP() << "Test file subject_es_1.pdf not found at " << pdf_path;
    }

    TextFileHandler handler(pdf_path.string());
    handler.parse();

    auto italic_texts = handler.find_italic();
    EXPECT_FALSE(italic_texts.empty()) << "Expected italic text segments";
}

TEST_F(ExtensionTypePdfTest, GetFontSizes) {
    if (!fs::exists(pdf_path)) {
        GTEST_SKIP() << "Test file subject_es_1.pdf not found at " << pdf_path;
    }

    TextFileHandler handler(pdf_path.string());
    handler.parse();

    auto sizes = handler.get_font_sizes();
    EXPECT_FALSE(sizes.empty());
    for (float size : sizes) {
        EXPECT_GT(size, 0.0f);
    }
}

TEST_F(ExtensionTypePdfTest, HighlightedColorsAndTexts) {
    if (!fs::exists(pdf_path)) {
        GTEST_SKIP() << "Test file subject_es_1.pdf not found at " << pdf_path;
    }

    TextFileHandler handler(pdf_path.string());
    handler.parse();

    // 1. Get the list of all unique highlight colors
    auto colors = handler.get_highlighted_colors();
    EXPECT_FALSE(colors.empty()) << "Expected highlighted colors in the document";

    auto it1 = std::find(colors.begin(), colors.end(), 0x000000);
    EXPECT_EQ(it1, colors.end()) << "Color 0x000000 was not expected to be found in the document highlights";

    auto it2 = std::find(colors.begin(), colors.end(), 0x0000FF);
    EXPECT_EQ(it2, colors.end()) << "Color 0x0000FF was not expected to be found in the document highlights";

    auto it3 = std::find(colors.begin(), colors.end(), 0x00FF00);
    EXPECT_NE(it3, colors.end()) << "Color 0x00FF00 is expected to be found in the document highlights";
    std::vector<std::string> expected_green = {"algoritmos de aprendizaje automático"};
    EXPECT_EQ(handler.find_highlight(0x00FF00), expected_green);

    auto it4 = std::find(colors.begin(), colors.end(), 0x00FFFF);
    EXPECT_NE(it4, colors.end()) << "Color 0x00FFFF is expected to be found in the document highlights";
    std::vector<std::string> expected_blue = {
        "correo electrónico",
        "plataformas de \nvideoconferencia,",
        "las redes sociales",
        "sistemas de gestión empresarial"
    };
    EXPECT_EQ(handler.find_highlight(0x00FFFF), expected_blue);

    auto it5 = std::find(colors.begin(), colors.end(), 0xFF0000);
    EXPECT_NE(it5, colors.end()) << "Color 0xFF0000 is expected to be found in the document highlights";
    std::vector<std::string> expected_red = {"la toma de decisiones o el procesamiento del lenguaje natural"};
    EXPECT_EQ(handler.find_highlight(0xFF0000), expected_red);

    auto it6 = std::find(colors.begin(), colors.end(), 0xFF00FF);
    EXPECT_EQ(it6, colors.end()) << "Color 0xFF00FF was not expected to be found in the document highlights";

    auto it7 = std::find(colors.begin(), colors.end(), 0xFFFF00);
    EXPECT_NE(it7, colors.end()) << "Color 0xFFFF00 is expected to be found in the document highlights";
    std::vector<std::string> expected_yellow = {"organización social y económica"};
    EXPECT_EQ(handler.find_highlight(0xFFFF00), expected_yellow);

    auto it8 = std::find(colors.begin(), colors.end(), 0xFFFFFF);
    EXPECT_EQ(it8, colors.end()) << "Color 0xFFFFFF was not expected to be found in the document highlights";
}
