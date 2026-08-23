#ifndef MOCK_GPU_DETECTOR_HPP
#define MOCK_GPU_DETECTOR_HPP

#include <gmock/gmock.h>
#include "speech_recognition/gpu_detector.hpp"

namespace cantatema::infra {

class MockGpuDetector : public IGpuDetector {
public:
    MockGpuDetector() = default;
    ~MockGpuDetector() override = default;

    MOCK_METHOD(AccelerationReport, detect_accelerators, (), (override));
    MOCK_METHOD(cantatema::HardwareInfo, detect_hardware, (), (override));
    MOCK_METHOD(cantatema::CpuInfo, detect_cpu, (), (override));
};

} // namespace cantatema::infra

#endif // MOCK_GPU_DETECTOR_HPP
