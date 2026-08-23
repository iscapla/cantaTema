/**
 * @file gpu_detector.hpp
 * @brief Utility for probing GPU accelerators (CUDA, Vulkan, Metal) and reporting
 *        device capabilities for whisper.cpp inference.
 */

#pragma once

#include <cstddef>
#include <string>
#include <vector>
#include <memory>

#include "primitives/hardware_info.hpp"

namespace cantatema::infra {

/**
 * @brief Detailed properties of a single detected hardware device in GGML.
 */
struct DeviceDetails {
    std::string name;             ///< Short name / identifier (e.g. "CUDA0", "CPU")
    std::string description;      ///< Human-readable description (e.g. "NVIDIA GeForce RTX 3080")
    std::string backend_name;     ///< Backend registry name (e.g. "CUDA", "Vulkan", "Metal", "CPU")
    std::string type_str;         ///< Category string ("GPU", "Integrated GPU", "CPU", "Accelerator")
    bool        is_gpu = false;   ///< True if device is dedicated or integrated GPU
    std::size_t memory_free_mb  = 0;
    std::size_t memory_total_mb = 0;
};

/**
 * @brief Summary report of available hardware acceleration options on the host machine.
 */
struct AccelerationReport {
    std::vector<DeviceDetails> devices;
    bool        has_cuda     = false;
    bool        has_vulkan   = false;
    bool        has_metal    = false;
    bool        has_any_gpu  = false;
    std::string selected_device_name;
    bool        use_gpu      = false;
};

/**
 * @class IGpuDetector
 * @brief Abstract interface for hardware accelerator detection and capability reporting.
 */
class IGpuDetector {
public:
    virtual ~IGpuDetector() = default;

    /**
     * @brief Probe host hardware and registered GGML backends to generate an acceleration report.
     * @return AccelerationReport containing enumerated devices and selected backend strategy.
     */
    virtual AccelerationReport detect_accelerators() = 0;

    /**
     * @brief Probe host hardware and compile full CPU and GPU device report.
     * @return cantatema::HardwareInfo Aggregated hardware detection profile.
     */
    virtual cantatema::HardwareInfo detect_hardware() = 0;

    /**
     * @brief Probe host CPU specifications (model brand, architecture, logical cores, system RAM).
     * @return cantatema::CpuInfo Detected CPU profile.
     */
    virtual cantatema::CpuInfo detect_cpu() = 0;
};

/**
 * @class GpuDetector
 * @brief Concrete implementation of IGpuDetector probing host CUDA/Vulkan/Metal drivers, GGML backends, and CPU.
 */
class GpuDetector : public IGpuDetector {
public:
    GpuDetector() = default;
    ~GpuDetector() override = default;

    AccelerationReport detect_accelerators() override;
    cantatema::HardwareInfo detect_hardware() override;
    cantatema::CpuInfo detect_cpu() override;
};

/**
 * @brief Probe all registered GGML backend devices and generate a hardware acceleration report.
 * @return AccelerationReport containing enumerated devices and selected backend strategy.
 */
AccelerationReport detect_accelerators();

/**
 * @brief Probe host CPU and GPU devices to generate a comprehensive HardwareInfo report.
 * @return cantatema::HardwareInfo Aggregated host hardware details.
 */
cantatema::HardwareInfo detect_hardware();

/**
 * @brief Probe host CPU specifications (model brand, architecture, logical cores, system RAM).
 * @return cantatema::CpuInfo Detected CPU profile.
 */
cantatema::CpuInfo detect_cpu();

} // namespace cantatema::infra
