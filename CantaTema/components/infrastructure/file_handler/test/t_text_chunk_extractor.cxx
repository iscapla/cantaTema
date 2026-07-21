#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "file_handler/text_handler.hpp"
#include "file_handler/text_chunk_extractor.hpp"
#include "configuration/configuration_system.hpp"

namespace fs = std::filesystem;

class TextChunkExtractorTest : public ::testing::Test {
protected:
    fs::path test_dir;
    fs::path pdf_path;

    void SetUp() override {
        test_dir = fs::temp_directory_path() / "canta_tema_extractor_tests";
        if (fs::exists(test_dir)) {
            fs::remove_all(test_dir);
        }
        fs::create_directories(test_dir);
        
        pdf_path = fs::path("example_data") / "subject_es_1.pdf";
    }

    void TearDown() override {
        if (fs::exists(test_dir)) {
            fs::remove_all(test_dir);
        }
    }

    // Helper to write a simple text file
    fs::path write_temp_text_file(const std::string& filename, const std::string& content) {
        fs::path file_path = test_dir / filename;
        std::ofstream ofs(file_path);
        ofs << content;
        return file_path;
    }
};

TEST_F(TextChunkExtractorTest, SentenceChunkingWithAbbreviations) {
    std::string text = "Esta es la pág. 15 del temario. Y este es el ej. práctico! ¿Será verdad? ¡Sí, claro! Fin de la línea\nNueva sección.";
    fs::path txt_file = write_temp_text_file("abbr_test.txt", text);

    TextFileHandler handler(txt_file.string());
    handler.parse();

    TextChunkExtractor extractor;
    std::vector<DocumentChunk> chunks;
    rst_code_e rst = extractor.extract_chunks(handler, chunks);

    EXPECT_EQ(rst, RST_OK);
    ASSERT_EQ(chunks.size(), 6);
    EXPECT_EQ(chunks[0].text, "Esta es la pág. 15 del temario.");
    EXPECT_EQ(chunks[1].text, "Y este es el ej. práctico!");
    EXPECT_EQ(chunks[2].text, "¿Será verdad?");
    EXPECT_EQ(chunks[3].text, "¡Sí, claro!");
    EXPECT_EQ(chunks[4].text, "Fin de la línea");
    EXPECT_EQ(chunks[5].text, "Nueva sección.");

    for (const auto& chunk : chunks) {
        EXPECT_FALSE(chunk.is_bold);
        EXPECT_FALSE(chunk.is_italic);
        EXPECT_DOUBLE_EQ(chunk.importance_weight, 1.0);
    }
}

TEST_F(TextChunkExtractorTest, EmptyFileTriggersError) {
    fs::path empty_file = write_temp_text_file("empty.txt", "   \n  \t   ");

    TextFileHandler handler(empty_file.string());
    handler.parse();

    TextChunkExtractor extractor;
    std::vector<DocumentChunk> chunks;
    rst_code_e rst = extractor.extract_chunks(handler, chunks);

    EXPECT_EQ(rst, FILE_EMPTY_OR_INVALID);
}

TEST_F(TextChunkExtractorTest, PageLimitTrigger) {
    fs::path txt_file = write_temp_text_file("limit_test.txt", "Una frase simple.");

    TextFileHandler handler(txt_file.string());
    handler.parse();

    ConfigurationSystem& config = ConfigurationSystem::getInstance();
    unsigned int original_limit = config.get_max_pdf_page_count();
    
    config.set_value("USER_LIMITS", "max_pdf_page_count", "0");

    TextChunkExtractor extractor;
    std::vector<DocumentChunk> chunks;
    rst_code_e rst = extractor.extract_chunks(handler, chunks);

    EXPECT_EQ(rst, FILE_EXCEEDS_PAGE_LIMIT);

    config.set_value("USER_LIMITS", "max_pdf_page_count", std::to_string(original_limit));
}

TEST_F(TextChunkExtractorTest, PdfRichTextFormattingAndWeights) {
    if (!fs::exists(pdf_path)) {
        GTEST_SKIP() << "Test file subject_es_1.pdf not found at " << pdf_path;
    }

    TextFileHandler handler(pdf_path.string());
    handler.parse();

    TextChunkExtractor extractor;
    std::vector<DocumentChunk> chunks;
    rst_code_e rst = extractor.extract_chunks(handler, chunks);

    EXPECT_EQ(rst, RST_OK);
    EXPECT_FALSE(chunks.empty());

    bool found_bold = false;
    bool found_italic = false;
    bool found_weighted = false;

    for (const auto& chunk : chunks) {
        if (chunk.is_bold) found_bold = true;
        if (chunk.is_italic) found_italic = true;
        if (chunk.importance_weight > 1.0) found_weighted = true;
    }

    EXPECT_TRUE(found_bold) << "Should detect bold chunks in example PDF";
    EXPECT_TRUE(found_italic) << "Should detect italic chunks in example PDF";
    EXPECT_TRUE(found_weighted) << "Should compute weights > 1.0 for formatted chunks";
}
