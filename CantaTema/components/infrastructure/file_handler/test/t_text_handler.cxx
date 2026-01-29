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
    
    rst_code_e result = handler.upload_file(dest_path.string());

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
    rst_code_e result = handler.upload_file(dest_path.string());

    EXPECT_NE(result, RST_OK);
    EXPECT_FALSE(fs::exists(dest_path));
}
