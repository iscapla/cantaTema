#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "speech_recognition/gpu_detector.hpp"
#include "speech_recognition/mocks/mock_gpu_detector.hpp"

using ::testing::Return;
using ::testing::StrictMock;
using cantatema::infra::AccelerationReport;
using cantatema::infra::DeviceDetails;
using cantatema::infra::GpuDetector;
using cantatema::infra::MockGpuDetector;

TEST(GpuDetectorTest, MockCudaAccelerationReport) {
    StrictMock<MockGpuDetector> mock_detector;

    AccelerationReport mock_report;
    mock_report.has_cuda = true;
    mock_report.has_any_gpu = true;
    mock_report.use_gpu = true;
    mock_report.selected_device_name = "CUDA";

    DeviceDetails cuda_dev;
    cuda_dev.name = "CUDA0";
    cuda_dev.backend_name = "CUDA";
    cuda_dev.description = "NVIDIA GeForce RTX 3080";
    cuda_dev.is_gpu = true;
    mock_report.devices.push_back(cuda_dev);

    EXPECT_CALL(mock_detector, detect_accelerators())
        .WillOnce(Return(mock_report));

    auto report = mock_detector.detect_accelerators();
    EXPECT_TRUE(report.has_cuda);
    EXPECT_TRUE(report.use_gpu);
    EXPECT_EQ(report.selected_device_name, "CUDA");
    EXPECT_FALSE(report.devices.empty());
}

TEST(GpuDetectorTest, MockVulkanAccelerationReport) {
    StrictMock<MockGpuDetector> mock_detector;

    AccelerationReport mock_report;
    mock_report.has_vulkan = true;
    mock_report.has_any_gpu = true;
    mock_report.use_gpu = true;
    mock_report.selected_device_name = "Vulkan";

    EXPECT_CALL(mock_detector, detect_accelerators())
        .WillOnce(Return(mock_report));

    auto report = mock_detector.detect_accelerators();
    EXPECT_TRUE(report.has_vulkan);
    EXPECT_TRUE(report.use_gpu);
    EXPECT_EQ(report.selected_device_name, "Vulkan");
}

TEST(GpuDetectorTest, MockCpuFallbackReport) {
    StrictMock<MockGpuDetector> mock_detector;

    AccelerationReport mock_report;
    mock_report.has_any_gpu = false;
    mock_report.use_gpu = false;
    mock_report.selected_device_name = "CPU";

    EXPECT_CALL(mock_detector, detect_accelerators())
        .WillOnce(Return(mock_report));

    auto report = mock_detector.detect_accelerators();
    EXPECT_FALSE(report.use_gpu);
    EXPECT_EQ(report.selected_device_name, "CPU");
}

TEST(GpuDetectorTest, ConcreteGpuDetectorExecutionDoesNotCrash) {
    GpuDetector detector;
    auto report = detector.detect_accelerators();
    EXPECT_FALSE(report.selected_device_name.empty());
}

TEST(GpuDetectorTest, ConcreteDetectCpuReturnsValidProfile) {
    GpuDetector detector;
    auto cpu = detector.detect_cpu();
    EXPECT_FALSE(cpu.name.empty());
    EXPECT_FALSE(cpu.architecture.empty());
    EXPECT_GT(cpu.core_count, 0u);
}

TEST(GpuDetectorTest, ConcreteDetectHardwareReturnsValidReport) {
    GpuDetector detector;
    auto hw = detector.detect_hardware();
    EXPECT_FALSE(hw.cpu.name.empty());
    EXPECT_GT(hw.cpu.core_count, 0u);
    EXPECT_FALSE(hw.selected_backend.empty());
}

TEST(GpuDetectorTest, FreeFunctionsExecuteSuccessfully) {
    auto cpu = cantatema::infra::detect_cpu();
    EXPECT_FALSE(cpu.name.empty());

    auto hw = cantatema::infra::detect_hardware();
    EXPECT_FALSE(hw.cpu.name.empty());

    auto accel = cantatema::infra::detect_accelerators();
    EXPECT_FALSE(accel.selected_device_name.empty());
}

TEST(GpuDetectorTest, MockHardwareReport) {
    StrictMock<MockGpuDetector> mock_detector;

    cantatema::HardwareInfo mock_hw;
    mock_hw.cpu.name = "Mock Processor 8-Core";
    mock_hw.cpu.architecture = "x86_64";
    mock_hw.cpu.core_count = 8;
    mock_hw.cpu.system_ram_mb = 16384;
    mock_hw.has_cuda = true;
    mock_hw.has_any_gpu = true;
    mock_hw.use_gpu = true;
    mock_hw.selected_backend = "CUDA";

    cantatema::GpuInfo gpu;
    gpu.name = "CUDA0";
    gpu.description = "Mock RTX 4090";
    gpu.backend_name = "CUDA";
    gpu.type_str = "GPU (Dedicated)";
    gpu.is_gpu = true;
    gpu.memory_total_mb = 24576;
    gpu.memory_free_mb = 20480;
    mock_hw.gpus.push_back(gpu);

    EXPECT_CALL(mock_detector, detect_hardware())
        .WillOnce(Return(mock_hw));

    auto hw = mock_detector.detect_hardware();
    EXPECT_EQ(hw.cpu.name, "Mock Processor 8-Core");
    EXPECT_EQ(hw.cpu.core_count, 8u);
    EXPECT_TRUE(hw.has_cuda);
    EXPECT_TRUE(hw.use_gpu);
    ASSERT_EQ(hw.gpus.size(), 1u);
    EXPECT_EQ(hw.gpus[0].description, "Mock RTX 4090");
    EXPECT_EQ(hw.gpus[0].memory_total_mb, 24576u);
}

