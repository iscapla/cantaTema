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
