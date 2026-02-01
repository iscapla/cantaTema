#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <string>

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
