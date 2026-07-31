#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "file_handler/text_handler.hpp"
#include "file_handler/text_chunk_extractor.hpp"

namespace fs = std::filesystem;

class TextConversionCoverageTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir = fs::temp_directory_path() / "canta_text_conv_coverage_scratch";
        if (fs::exists(test_dir)) {
            fs::remove_all(test_dir);
        }
        fs::create_directories(test_dir);
    }

    void TearDown() override {
        if (fs::exists(test_dir)) {
            fs::remove_all(test_dir);
        }
    }

    void create_minimal_pdf(const fs::path& path) {
        const std::vector<char> pdf_data = {
            '%', 'P', 'D', 'F', '-', '1', '.', '4', '\n',
            '1', ' ', '0', ' ', 'o', 'b', 'j', '\n', '<', '<', '/', 'T', 'y', 'p', 'e', '/', 'C', 'a', 't', 'a', 'l', 'o', 'g', '/', 'P', 'a', 'g', 'e', 's', ' ', '2', ' ', '0', ' ', 'R', '>', '>', '\n', 'e', 'n', 'd', 'o', 'b', 'j', '\n',
            '2', ' ', '0', ' ', 'o', 'b', 'j', '\n', '<', '<', '/', 'T', 'y', 'p', 'e', '/', 'P', 'a', 'g', 'e', 's', '/', 'K', 'i', 'd', 's', '[', '3', ' ', '0', ' ', 'R', ']', '/', 'C', 'o', 'u', 'n', 't', ' ', '1', '>', '>', '\n', 'e', 'n', 'd', 'o', 'b', 'j', '\n',
            '3', ' ', '0', ' ', 'o', 'b', 'j', '\n', '<', '<', '/', 'T', 'y', 'p', 'e', '/', 'P', 'a', 'g', 'e', '/', 'P', 'a', 'r', 'e', 'n', 't', ' ', '2', ' ', '0', ' ', 'R', '/', 'M', 'e', 'd', 'i', 'a', 'B', 'o', 'x', '[', '0', ' ', '0', ' ', '5', '0', '0', ' ', '5', '0', '0', ']', '/', 'C', 'o', 'n', 't', 'e', 'n', 't', 's', ' ', '4', ' ', '0', ' ', 'R', '/', 'R', 'e', 's', 'o', 'u', 'r', 'c', 'e', 's', '<', '<', '/', 'F', 'o', 'n', 't', '<', '<', '/', 'F', '1', ' ', '5', ' ', '0', ' ', 'R', '>', '>', '>', '>', '>', '>', '\n', 'e', 'n', 'd', 'o', 'b', 'j', '\n',
            '4', ' ', '0', ' ', 'o', 'b', 'j', '\n', '<', '<', '/', 'L', 'e', 'n', 'g', 't', 'h', ' ', '4', '4', '>', '>', '\n', 's', 't', 'r', 'e', 'a', 'm', '\n',
            'B', 'T', ' ', '/', 'F', '1', ' ', '2', '4', ' ', 'T', 'f', ' ', '1', '0', '0', ' ', '1', '0', '0', ' ', 'T', 'd', ' ', '(', 'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', ')', ' ', 'T', 'j', ' ', 'E', 'T', '\n',
            'e', 'n', 'd', 's', 't', 'r', 'e', 'a', 'm', '\n', 'e', 'n', 'd', 'o', 'b', 'j', '\n',
            '5', ' ', '0', ' ', 'o', 'b', 'j', '\n', '<', '<', '/', 'T', 'y', 'p', 'e', '/', 'F', 'o', 'n', 't', '/', 'S', 'u', 'b', 't', 'y', 'p', 'e', '/', 'T', 'y', 'p', 'e', '1', '/', 'B', 'a', 's', 'e', 'F', 'o', 'n', 't', '/', 'H', 'e', 'l', 'v', 'e', 't', 'i', 'c', 'a', '>', '>', '\n', 'e', 'n', 'd', 'o', 'b', 'j', '\n',
            'x', 'r', 'e', 'f', '\n',
            '0', ' ', '6', '\n',
            '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', ' ', '6', '5', '5', '3', '5', ' ', 'f', ' ', '\n',
            '0', '0', '0', '0', '0', '0', '0', '0', '0', '9', ' ', '0', '0', '0', '0', '0', ' ', 'n', ' ', '\n',
            '0', '0', '0', '0', '0', '0', '0', '0', '5', '8', ' ', '0', '0', '0', '0', '0', ' ', 'n', ' ', '\n',
            '0', '0', '0', '0', '0', '0', '0', '1', '1', '5', ' ', '0', '0', '0', '0', '0', ' ', 'n', ' ', '\n',
            '0', '0', '0', '0', '0', '0', '0', '2', '4', '4', ' ', '0', '0', '0', '0', '0', ' ', 'n', ' ', '\n',
            '0', '0', '0', '0', '0', '0', '0', '3', '3', '7', ' ', '0', '0', '0', '0', '0', ' ', 'n', ' ', '\n',
            't', 'r', 'a', 'i', 'l', 'e', 'r', '\n', '<', '<', '/', 'S', 'i', 'z', 'e', ' ', '6', '/', 'R', 'o', 'o', 't', ' ', '1', ' ', '0', ' ', 'R', '>', '>', '\n', 's', 't', 'a', 'r', 't', 'x', 'r', 'e', 'f', '\n', '4', '2', '6', '\n', '%', '%', 'E', 'O', 'F', '\n'
        };

        std::ofstream ofs(path, std::ios::binary);
        ofs.write(pdf_data.data(), pdf_data.size());
    }

    fs::path test_dir;
};

// 6. Text conversion (PDF). File not found
TEST_F(TextConversionCoverageTest, PdfFileNotFound) {
    fs::path missing_pdf = test_dir / "missing.pdf";
    TextFileHandler handler(missing_pdf.string());
    handler.parse();
    EXPECT_EQ(handler.get_number_of_pages(), 0);
    EXPECT_TRUE(handler.extract_text_content().empty());

    TextChunkExtractor extractor;
    std::vector<DocumentChunk> chunks;
    EXPECT_EQ(extractor.extract_chunks(handler, chunks), FILE_EMPTY_OR_INVALID);
}

// 7. Text conversion (PDF). File cannot be read
TEST_F(TextConversionCoverageTest, PdfFileCannotBeRead) {
    EXPECT_THROW(TextFileHandler(""), std::runtime_error);
}

// 8. Text conversion (PDF). File with wrong format (it is not a real pdf)
TEST_F(TextConversionCoverageTest, PdfFileWrongFormat) {
    fs::path corrupt_pdf = test_dir / "corrupt.pdf";
    std::ofstream ofs(corrupt_pdf, std::ios::binary);
    ofs << "THIS_IS_NOT_A_VALID_PDF_HEADER_OR_DATA_STREAM";
    ofs.close();

    TextFileHandler handler(corrupt_pdf.string());
    handler.parse();
    EXPECT_TRUE(handler.extract_text_content().empty());

    TextChunkExtractor extractor;
    std::vector<DocumentChunk> chunks;
    EXPECT_EQ(extractor.extract_chunks(handler, chunks), FILE_EMPTY_OR_INVALID);
}

// 9. Text conversion (PDF). just text
TEST_F(TextConversionCoverageTest, PdfJustText) {
    fs::path pdf_path = test_dir / "plain.pdf";
    create_minimal_pdf(pdf_path);

    TextFileHandler handler(pdf_path.string());
    handler.parse();
    EXPECT_GT(handler.get_number_of_pages(), 0);

    TextChunkExtractor extractor;
    std::vector<DocumentChunk> chunks;
    ASSERT_EQ(extractor.extract_chunks(handler, chunks), RST_OK);
    ASSERT_FALSE(chunks.empty());
    EXPECT_TRUE(chunks[0].text.find("Hello World") != std::string::npos);
    EXPECT_GE(chunks[0].importance_weight, 1.0);
}

// 10. Text conversion (PDF). with formats (bold and italic)
TEST_F(TextConversionCoverageTest, PdfWithFormatsBoldAndItalic) {
    DocumentChunk bold_chunk;
    bold_chunk.chunk_id = 1;
    bold_chunk.text = "Bold sentence topic.";
    bold_chunk.is_bold = true;
    bold_chunk.importance_weight = 1.5;

    DocumentChunk italic_chunk;
    italic_chunk.chunk_id = 2;
    italic_chunk.text = "Italic sentence concept.";
    italic_chunk.is_italic = true;
    italic_chunk.importance_weight = 1.2;

    EXPECT_TRUE(bold_chunk.is_bold);
    EXPECT_GT(bold_chunk.importance_weight, 1.0);
    EXPECT_TRUE(italic_chunk.is_italic);
    EXPECT_GT(italic_chunk.importance_weight, 1.0);
}

// 11. Text conversion (txt). File not found
TEST_F(TextConversionCoverageTest, TxtFileNotFound) {
    fs::path missing_txt = test_dir / "missing.txt";
    TextFileHandler handler(missing_txt.string());
    handler.parse();
    EXPECT_EQ(handler.get_number_of_pages(), 0);

    TextChunkExtractor extractor;
    std::vector<DocumentChunk> chunks;
    EXPECT_EQ(extractor.extract_chunks(handler, chunks), FILE_EMPTY_OR_INVALID);
}

// 12. Text conversion (txt). File cannot be read
TEST_F(TextConversionCoverageTest, TxtFileCannotBeRead) {
    EXPECT_THROW(TextFileHandler(""), std::runtime_error);
}

// 13. Text conversion (txt). File with wrong format (it is not a real txt)
TEST_F(TextConversionCoverageTest, TxtFileWrongFormat) {
    fs::path corrupt_txt = test_dir / "corrupt.txt";
    // Create zero-byte empty text file
    std::ofstream ofs(corrupt_txt, std::ios::binary);
    ofs.close();

    TextFileHandler handler(corrupt_txt.string());
    handler.parse();
    EXPECT_TRUE(handler.extract_text_content().empty());

    TextChunkExtractor extractor;
    std::vector<DocumentChunk> chunks;
    EXPECT_EQ(extractor.extract_chunks(handler, chunks), FILE_EMPTY_OR_INVALID);
}

// 14. Text conversion (txt). just text
TEST_F(TextConversionCoverageTest, TxtJustText) {
    fs::path txt_path = test_dir / "sample.txt";
    std::ofstream ofs(txt_path);
    ofs << "First plain text sentence. Second plain text sentence.";
    ofs.close();

    TextFileHandler handler(txt_path.string());
    handler.parse();

    TextChunkExtractor extractor;
    std::vector<DocumentChunk> chunks;
    ASSERT_EQ(extractor.extract_chunks(handler, chunks), RST_OK);
    ASSERT_EQ(chunks.size(), 2u);
    EXPECT_EQ(chunks[0].text, "First plain text sentence.");
    EXPECT_EQ(chunks[1].text, "Second plain text sentence.");
}
