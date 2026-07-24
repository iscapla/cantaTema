/**
 * @file gpu_detector.hpp
 * @brief Utility for probing GPU accelerators (CUDA, Vulkan, Metal) and reporting
 *        device capabilities for whisper.cpp inference.
 */

#pragma once

#include <cstddef>
#include <string>
#include <vector>

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
 * @brief Probe all registered GGML backend devices and generate a hardware acceleration report.
 *
 * Log outputs are generated at info/debug levels detailing detected devices and
 * the recommended acceleration strategy.
 *
 * @return AccelerationReport containing enumerated devices and selected backend strategy.
 */
AccelerationReport detect_accelerators();

} // namespace cantatema::infra
