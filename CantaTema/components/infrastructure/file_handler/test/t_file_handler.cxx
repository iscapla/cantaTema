#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <numeric>

#include "file_handler/file_handler.hpp"

namespace fs = std::filesystem;

// Helper class to access protected members for testing
class TestableFileHandler : public FileHandler {
public:
    using FileHandler::read_and_stream;
    using FileHandler::save_chunk;
    using FileHandler::FileHandler;
};

class FileHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a unique temporary directory for this test run
        test_dir = fs::temp_directory_path() / "canta_tema_file_handler_tests";
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
    TestableFileHandler handler;
};

TEST_F(FileHandlerTest, IsPathValidChecks) {
    fs::path valid_path = test_dir / "valid.txt";
    {
        std::ofstream ofs(valid_path);
        ofs << "content";
    }
    TestableFileHandler h1(valid_path.string(), 0);
    EXPECT_TRUE(h1.is_file_path_valid());

    TestableFileHandler h2("", 0);
    EXPECT_FALSE(h2.is_file_path_valid());
}

TEST_F(FileHandlerTest, SaveChunkCreatesFile) {
    fs::path file_path = test_dir / "test_file.txt";
    std::string content = "Hello World";
    std::vector<char> data(content.begin(), content.end());

    rst_code_e result = handler.save_chunk(file_path.string(), data, true);
    
    EXPECT_EQ(result, RST_OK);
    EXPECT_TRUE(fs::exists(file_path));
    EXPECT_EQ(fs::file_size(file_path), content.size());
}

TEST_F(FileHandlerTest, SaveChunkAppendsData) {
    fs::path file_path = test_dir / "append_test.txt";
    std::string part1 = "Part 1";
    std::string part2 = "Part 2";
    
    std::vector<char> data1(part1.begin(), part1.end());
    std::vector<char> data2(part2.begin(), part2.end());

    EXPECT_EQ(handler.save_chunk(file_path.string(), data1, true), RST_OK);
    EXPECT_EQ(handler.save_chunk(file_path.string(), data2, false), RST_OK);

    std::ifstream ifs(file_path, std::ios::binary);
    std::string file_content((std::istreambuf_iterator<char>(ifs)),
                             (std::istreambuf_iterator<char>()));
    
    EXPECT_EQ(file_content, part1 + part2);
}

TEST_F(FileHandlerTest, ReadAndStreamReadsCorrectly) {
    fs::path file_path = test_dir / "read_test.bin";
    std::vector<char> original_data(1000);
    std::iota(original_data.begin(), original_data.end(), 0);

    // Write manually to ensure setup is correct independent of save_chunk
    {
        std::ofstream ofs(file_path, std::ios::binary);
        ofs.write(original_data.data(), original_data.size());
    }

    TestableFileHandler read_handler(file_path.string(), 0);
    std::vector<char> read_data;
    auto callback = [&](const std::vector<char>& chunk) -> rst_code_e {
        read_data.insert(read_data.end(), chunk.begin(), chunk.end());
        return RST_OK;
    };

    rst_code_e result = read_handler.read_and_stream(callback);

    EXPECT_EQ(result, RST_OK);
    EXPECT_EQ(read_data, original_data);
}

TEST_F(FileHandlerTest, RemoveFileDeletesFile) {
    fs::path file_path = test_dir / "delete_me.txt";
    {
        std::ofstream ofs(file_path);
        ofs << "bye";
    }
    ASSERT_TRUE(fs::exists(file_path));

    TestableFileHandler remove_handler(file_path.string(), 0);
    rst_code_e result = remove_handler.remove_file();
    
    EXPECT_EQ(result, RST_OK);
    EXPECT_FALSE(fs::exists(file_path));
}

TEST_F(FileHandlerTest, RemoveFolderDeletesRecursive) {
    fs::path folder_path = test_dir / "subfolder";
    fs::create_directories(folder_path);
    
    fs::path file_in_folder = folder_path / "nested.txt";
    {
        std::ofstream ofs(file_in_folder);
        ofs << "nested";
    }

    ASSERT_TRUE(fs::exists(folder_path));
    ASSERT_TRUE(fs::exists(file_in_folder));

    rst_code_e result = handler.remove_folder(folder_path.string());

    EXPECT_EQ(result, RST_OK);
    EXPECT_FALSE(fs::exists(folder_path));
}