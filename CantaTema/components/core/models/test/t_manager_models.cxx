#include <gtest/gtest.h>
#include <fstream>
#include <cstdio>
#include <vector>
#include <string>
#include <algorithm>
#include <filesystem>

#include "models/manager_models.hpp"
#include "primitives/tool_paths.hpp"

class ManagerModelsTest : public ::testing::Test {
protected:
    const std::string test_whisper_name = "test_local_whisper";
    const std::string test_whisper_filename = "ggml-test_local_whisper.bin";
    const std::string test_llama_name = "test_local_llama";
    const std::string test_llama_filename = "test_local_llama.gguf";

    std::filesystem::path whisper_path;
    std::filesystem::path llama_path;

    void SetUp() override {
        whisper_path = ToolPath::get_path_for_models_whisper() / test_whisper_filename;
        llama_path = ToolPath::get_path_for_models_llama() / test_llama_filename;

        std::filesystem::create_directories(whisper_path.parent_path());
        std::filesystem::create_directories(llama_path.parent_path());

        std::ofstream ws_file(whisper_path);
        ws_file << "dummy whisper content";
        ws_file.close();

        std::ofstream ll_file(llama_path);
        ll_file << "dummy llama content";
        ll_file.close();
    }

    void TearDown() override {
        std::filesystem::remove(whisper_path);
        std::filesystem::remove(llama_path);
    }
};

TEST_F(ManagerModelsTest, LocalIsModelAvailable) {
    ManagerModels manager;
    
    EXPECT_EQ(manager.local_is_whisper_model_available(test_whisper_name), RST_OK);
    EXPECT_NE(manager.local_is_whisper_model_available("non_existent_whisper"), RST_OK);

    EXPECT_EQ(manager.local_is_llama_model_available(test_llama_name), RST_OK);
    EXPECT_NE(manager.local_is_llama_model_available("non_existent_llama"), RST_OK);
}

TEST_F(ManagerModelsTest, LocalGetAvailableModels) {
    ManagerModels manager;
    std::vector<ManagerModels::ModelInfo> models;
    
    EXPECT_EQ(manager.get_available_models(false, models), RST_OK);
    
    bool whisper_found = false;
    bool llama_found = false;
    for (const auto& m : models) {
        if (m.name == test_whisper_name && m.type == ModelType::Whisper) {
            whisper_found = m.available_local;
        }
        if (m.name == test_llama_name && m.type == ModelType::Llama) {
            llama_found = m.available_local;
        }
    }
    EXPECT_TRUE(whisper_found);
    EXPECT_TRUE(llama_found);
}

TEST_F(ManagerModelsTest, NetworkIsModelAvailable) {
    ManagerModels manager;
    
    EXPECT_EQ(manager.network_is_model_available(ModelType::Whisper, "tiny"), RST_OK);
    EXPECT_NE(manager.network_is_model_available(ModelType::Whisper, "invalid_model_123"), RST_OK);

    EXPECT_EQ(manager.network_is_model_available(ModelType::Llama, "multilingual-e5-large-q8_0"), RST_OK);
    EXPECT_NE(manager.network_is_model_available(ModelType::Llama, "invalid_llama_123"), RST_OK);
}

TEST_F(ManagerModelsTest, AutoSelectWhisper) {
    ManagerModels manager;
    
    // Create base and small dummy files to verify auto selection logic
    std::filesystem::path small_path = ToolPath::get_path_for_models_whisper() / "ggml-small.bin";
    std::filesystem::path base_path = ToolPath::get_path_for_models_whisper() / "ggml-base.bin";
    
    std::ofstream(small_path) << "dummy";
    std::ofstream(base_path) << "dummy";

    // small is preferred over base
    EXPECT_EQ(manager.auto_select_whisper_model(), "small");

    std::filesystem::remove(small_path);
    // base is preferred over tiny
    EXPECT_EQ(manager.auto_select_whisper_model(), "base");

    std::filesystem::remove(base_path);
}

TEST_F(ManagerModelsTest, AutoSelectLlama) {
    ManagerModels manager;

    std::filesystem::path large_q8_path = ToolPath::get_path_for_models_llama() / "multilingual-e5-large-q8_0.gguf";
    std::filesystem::path large_f16_path = ToolPath::get_path_for_models_llama() / "multilingual-e5-large-f16.gguf";

    std::ofstream(large_q8_path) << "dummy";
    std::ofstream(large_f16_path) << "dummy";

    EXPECT_EQ(manager.auto_select_llama_model(), "multilingual-e5-large-q8_0");

    std::filesystem::remove(large_q8_path);
    EXPECT_EQ(manager.auto_select_llama_model(), "multilingual-e5-large-f16");

    std::filesystem::remove(large_f16_path);
}

TEST_F(ManagerModelsTest, DownloadModelFailure) {
    ManagerModels manager;
    
    auto dummy_callback = [](const DownloadProgress& progress) {};

    rst_code_e result = manager.network_download_model(ModelType::Whisper, "invalid_model_123", dummy_callback);
    EXPECT_EQ(result, MODELS_FILE_DOWNLOAD_FAIL);

    std::filesystem::path expected_temp = ToolPath::get_path_for_models_whisper() / "ggml-invalid_model_123.bin.tmp";
    EXPECT_FALSE(std::filesystem::exists(expected_temp));
}

TEST_F(ManagerModelsTest, NetworkGetAvailableLlamaModels) {
    ManagerModels manager;
    std::vector<ManagerModels::ModelInfo> llama_models;
    
    rst_code_e rst = manager.get_llama_models(true, llama_models);
    EXPECT_EQ(rst, RST_OK);
    EXPECT_FALSE(llama_models.empty());
    
    bool found_q8 = false;
    for (const auto& m : llama_models) {
        if (m.type == ModelType::Llama && m.name == "multilingual-e5-large-q8_0") {
            found_q8 = m.available_network;
        }
    }
    EXPECT_TRUE(found_q8);
}

