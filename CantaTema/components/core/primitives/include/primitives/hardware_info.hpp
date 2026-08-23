/**
 * @file hardware_info.hpp
 * @brief Data structures representing host CPU and GPU hardware detection information.
 */

#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace cantatema {

/**
 * @struct CpuInfo
 * @brief Detailed properties of host CPU processor.
 */
struct CpuInfo {
    std::string name;             ///< Processor brand / model string (e.g. "AMD Ryzen 7 5800X 8-Core Processor")
    std::string architecture;     ///< System architecture (e.g. "x86_64", "ARM64", "x86")
    unsigned int core_count = 0;  ///< Number of logical CPU threads / cores
    std::size_t system_ram_mb = 0;///< Total physical system RAM in Megabytes
};

/**
 * @struct GpuInfo
 * @brief Detailed properties of a detected GPU device.
 */
struct GpuInfo {
    std::string name;             ///< Short device identifier (e.g. "CUDA0", "Vulkan0", "DirectX0")
    std::string description;      ///< Human-readable description (e.g. "NVIDIA GeForce RTX 3080")
    std::string backend_name;     ///< Acceleration backend ("CUDA", "Vulkan", "Metal", "DirectX", "CPU")
    std::string type_str;         ///< Device category ("GPU (Dedicated)", "GPU (Integrated)", "CPU", "Accelerator")
    bool        is_gpu = false;   ///< True if device is a dedicated or integrated GPU
    std::size_t memory_free_mb = 0;  ///< Estimated available VRAM in Megabytes
    std::size_t memory_total_mb = 0; ///< Total VRAM in Megabytes
};

/**
 * @struct HardwareInfo
 * @brief Aggregated hardware profile reporting host CPU and GPU acceleration capabilities.
 */
struct HardwareInfo {
    CpuInfo              cpu;
    std::vector<GpuInfo> gpus;
    bool                 has_cuda = false;
    bool                 has_vulkan = false;
    bool                 has_metal = false;
    bool                 has_any_gpu = false;
    std::string          selected_backend; ///< Active/recommended AI acceleration backend
    bool                 use_gpu = false;  ///< True if GPU acceleration is enabled/supported
};

} // namespace cantatema
