#include <gtest/gtest.h>
#include <fstream>
#include <cstdio>
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>

#include "models/manager_whisper.hpp"
#include "primitives/tool_paths.hpp"

class ManagerWhisperTest : public ::testing::Test {
protected:
    const std::string test_model_name = "test_local";
    const std::string test_filename = "ggml-test_local.bin";
    std::filesystem::path full_path;

    void SetUp() override {
        // Create a dummy local model file for testing
        // The manager is expected to look for "ggml-{name}.bin"
        full_path = ToolPath::get_path_for_models_whisper() / test_filename;

        std::filesystem::create_directories(full_path.parent_path());

        std::ofstream outfile(full_path);
        outfile << "dummy content";
        outfile.close();
    }

    void TearDown() override {
        // Clean up the dummy file
        std::filesystem::remove(full_path);
    }
};

TEST_F(ManagerWhisperTest, LocalIsModelAvailable) {
    ManagerWhisper manager;
    
    // Check if the fake model is available
    EXPECT_EQ(manager.local_is_model_available(test_model_name), RST_OK);
    
    // Check for a non-existent model
    EXPECT_NE(manager.local_is_model_available("non_existent_model"), RST_OK);
}

TEST_F(ManagerWhisperTest, LocalGetAvailableModels) {
    ManagerWhisper manager;
    std::vector<std::string> models;
    
    EXPECT_EQ(manager.local_get_available_models(models), RST_OK);
    
    // Check if our test model is in the list
    auto it = std::find(models.begin(), models.end(), test_model_name);
    EXPECT_NE(it, models.end()) << "test_local model should be found locally";
}

TEST_F(ManagerWhisperTest, NetworkIsModelAvailable) {
    ManagerWhisper manager;
    
    // Check for a known standard model (tiny is usually available)
    EXPECT_EQ(manager.network_is_model_available("tiny"), RST_OK);
    
    // Check for a likely non-existent model
    EXPECT_NE(manager.network_is_model_available("invalid_model_xyz_123"), RST_OK);
}

TEST_F(ManagerWhisperTest, NetworkGetAvailableModels) {
    ManagerWhisper manager;
    std::vector<std::string> models;
    
    EXPECT_EQ(manager.network_get_available_models(models), RST_OK);
    
    // "tiny" should be in the list of available models from network
    auto it = std::find(models.begin(), models.end(), "tiny");
    EXPECT_NE(it, models.end()) << "tiny model should be available in network list";
}

TEST_F(ManagerWhisperTest, GetAvailableModelsCombined) {
    ManagerWhisper manager;
    std::vector<ManagerWhisper::WhisperModel> models;
    
    // 1. Check local only
    EXPECT_EQ(manager.get_available_models(false, models), RST_OK);
    
    bool local_found = false;
    for (const auto& m : models) {
        if (m.name == test_model_name) {
            EXPECT_TRUE(m.available_local);
            local_found = true;
        }
    }
    EXPECT_TRUE(local_found) << "test_local should be found in combined list (local check)";
    
    // 2. Check with network
    models.clear();
    EXPECT_EQ(manager.get_available_models(true, models), RST_OK);
    
    // We expect at least the local one to be there, and likely 'tiny' from network
    local_found = false;
    bool tiny_found = false;
    
    for (const auto& m : models) {
        if (m.name == test_model_name) {
            EXPECT_TRUE(m.available_local);
            local_found = true;
        }
        if (m.name == "tiny") {
            EXPECT_TRUE(m.available_network);
            tiny_found = true;
        }
    }
    EXPECT_TRUE(local_found) << "test_local should be found in combined list (network check)";
    EXPECT_TRUE(tiny_found) << "tiny should be found in combined list (network check)";
}

TEST_F(ManagerWhisperTest, DownloadModelFailure) {
    ManagerWhisper manager;
    
    // Define a dummy callback to exercise progress callback setup
    auto dummy_callback = [](const DownloadProgress& progress) {
        // Do nothing
    };

    // Attempt to download a non-existent model
    std::string invalid_model = "invalid_model_xyz_123";
    std::string expected_file = "ggml-invalid_model_xyz_123.bin";
    std::filesystem::path expected_path = ToolPath::get_path_for_models_whisper() / expected_file;

    rst_code_e result = manager.network_download_model(invalid_model, dummy_callback);
    EXPECT_EQ(result, MODELS_FILE_DOWNLOAD_FAIL);

    // Clean up if a partial/failed file was created
    if (std::filesystem::exists(expected_path)) {
        std::filesystem::remove(expected_path);
    }
}
