#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "file_handler/text_handler.hpp"

namespace fs = std::filesystem;

class TextHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a unique temporary directory for this test run
        test_dir = fs::temp_directory_path() / "canta_tema_text_handler_tests";
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

    // Helper to create a minimal valid PDF file containing "Hello World"
    // This avoids dependency on external files and ensures deterministic content.
    void create_minimal_pdf(const fs::path& path) {
        // Minimal PDF 1.4 structure with one page containing "Hello World"
        // Offsets (xref) are pre-calculated.
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

TEST_F(TextHandlerTest, UploadFileCopiesContent) {
    fs::path source_path = test_dir / "source.txt";
    fs::path dest_path = test_dir / "dest.txt";
    std::string content = "Text file content for upload test.";

    // Create the source file
    {
        std::ofstream ofs(source_path);
        ofs << content;
    }

    TextFileHandler handler(source_path.string());
    
    unsigned int uploaded_bytes;
    rst_code_e result = handler.upload_file(dest_path.string(), uploaded_bytes);

    EXPECT_EQ(result, RST_OK);
    EXPECT_TRUE(fs::exists(dest_path));
    
    // Verify content matches
    std::ifstream ifs(dest_path);
    std::string dest_content((std::istreambuf_iterator<char>(ifs)),
                             (std::istreambuf_iterator<char>()));
    EXPECT_EQ(dest_content, content);
}

TEST_F(TextHandlerTest, UploadFileFailsWhenSourceMissing) {
    fs::path source_path = test_dir / "non_existent_source.txt";
    fs::path dest_path = test_dir / "dest.txt";

    TextFileHandler handler(source_path.string());
    
    // Expect failure since source does not exist
    unsigned int uploaded_bytes;
    rst_code_e result = handler.upload_file(dest_path.string(), uploaded_bytes);

    EXPECT_NE(result, RST_OK);
    EXPECT_FALSE(fs::exists(dest_path));
}

TEST_F(TextHandlerTest, UploadFileReturnsCorrectSize) {
    fs::path source_path = test_dir / "size_test.txt";
    fs::path dest_path = test_dir / "dest_size.txt";
    std::string content = "1234567890";

    {
        std::ofstream ofs(source_path);
        ofs << content;
    }

    TextFileHandler handler(source_path.string());
    
    // Initialize with garbage to ensure it gets reset
    unsigned int uploaded_bytes = 12345;
    rst_code_e result = handler.upload_file(dest_path.string(), uploaded_bytes);

    EXPECT_EQ(result, RST_OK);
    EXPECT_EQ(uploaded_bytes, content.size());
}

TEST_F(TextHandlerTest, UploadFileHandlesLargeContent) {
    fs::path source_path = test_dir / "large_file.txt";
    fs::path dest_path = test_dir / "dest_large.txt";
    
    // Create content larger than the default chunk size (64KB) to force multi-chunk processing
    std::string content(70000, 'A'); 
    {
        std::ofstream ofs(source_path);
        ofs << content;
    }

    TextFileHandler handler(source_path.string());
    
    unsigned int uploaded_bytes = 0;
    rst_code_e result = handler.upload_file(dest_path.string(), uploaded_bytes);

    EXPECT_EQ(result, RST_OK);
    EXPECT_EQ(uploaded_bytes, content.size());
    EXPECT_EQ(fs::file_size(dest_path), content.size());
}

TEST_F(TextHandlerTest, ExtractTextContentFromPdf) {
    fs::path pdf_path = test_dir / "test.pdf";
    create_minimal_pdf(pdf_path);

    ASSERT_TRUE(fs::exists(pdf_path));

    TextFileHandler handler(pdf_path.string());
    handler.parse();
    std::string extracted = handler.extract_text_content();

    // Verify the extracted text contains the specific words from our generated PDF
    EXPECT_NE(extracted.find("Hello"), std::string::npos);
    EXPECT_NE(extracted.find("World"), std::string::npos);

    // Cover delegates and PDF-specific methods
    EXPECT_NO_THROW({
        handler.extract_rich_text();
        handler.find_bold();
        handler.find_italic();
        handler.find_highlight(0);
        handler.get_font_sizes();
        handler.get_highlighted_colors();
    });
}


TEST_F(TextHandlerTest, ExtractTextContentFromRealPdf) {
    fs::path source_file = __FILE__;
    fs::path project_root = source_file.parent_path().parent_path().parent_path().parent_path().parent_path();
    fs::path pdf_path = project_root / "example_data" / "subject_es_1.pdf";

    if (fs::exists(pdf_path)) {
        TextFileHandler handler(pdf_path.string());
        handler.parse();

        EXPECT_EQ(handler.get_number_of_pages(), 2);

        std::string extracted = handler.extract_text_content();
        EXPECT_FALSE(extracted.empty());
        EXPECT_NE(extracted.find("tecnología"), std::string::npos);
        EXPECT_NE(extracted.find("transformación"), std::string::npos);
    } else {
        GTEST_SKIP() << "Test file subject_es_1.pdf not found at " << pdf_path;
    }
}

TEST_F(TextHandlerTest, TXTFileParsingAndExtraction) {
    fs::path txt_path = test_dir / "sample.txt";
    std::string content = "This is a sample TXT content.\nWith another line.";
    {
        std::ofstream ofs(txt_path);
        ofs << content;
    }

    TextFileHandler handler(txt_path.string());
    
    // Test warnings or unparsed behaviour if any
    EXPECT_EQ(handler.extract_text_content(), "");

    handler.parse();
    
    // Parse again to cover early return branch (m_parsed == true)
    handler.parse();

    EXPECT_EQ(handler.get_number_of_pages(), 1);
    EXPECT_EQ(handler.extract_text_content(), content);
    
    // Check rich text spans
    auto spans = handler.extract_rich_text();
    ASSERT_EQ(spans.size(), 1);
    EXPECT_EQ(spans[0].text, content);

    // TXT file dummy method returns
    EXPECT_TRUE(handler.find_bold().empty());
    EXPECT_TRUE(handler.find_italic().empty());
    EXPECT_TRUE(handler.find_highlight(0).empty());
    EXPECT_TRUE(handler.get_font_sizes().empty());
    EXPECT_TRUE(handler.get_highlighted_colors().empty());
}

TEST_F(TextHandlerTest, TXTFileParsingEmptyFile) {
    fs::path txt_path = test_dir / "empty.txt";
    {
        std::ofstream ofs(txt_path);
    }

    TextFileHandler handler(txt_path.string());
    handler.parse();
    EXPECT_EQ(handler.get_number_of_pages(), 0);
    EXPECT_TRUE(handler.extract_rich_text().empty());
}

TEST_F(TextHandlerTest, TXTFileParsingInvalidPaths) {
    // Non-existent path
    TextFileHandler handler("does_not_exist_file_12345.txt");
    handler.parse();
    EXPECT_EQ(handler.get_number_of_pages(), 0);

    // Invalid extension exception
    EXPECT_THROW({
        TextFileHandler bad_handler("bad_extension.dat");
    }, std::runtime_error);

    EXPECT_THROW({
        TextFileHandler bad_handler2("no_dot_filename");
    }, std::runtime_error);

    EXPECT_THROW({
        TextFileHandler bad_handler3("");
    }, std::runtime_error);
}


