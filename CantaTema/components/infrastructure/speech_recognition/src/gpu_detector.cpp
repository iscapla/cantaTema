/**
 * @file gpu_detector.cpp
 * @brief Implementation of hardware accelerator enumeration and GPU probing.
 *        Includes native system CUDA driver probing via nvcuda/libcuda dynamic loading
 *        and GGML registered backend device enumeration.
 */

#include "speech_recognition/gpu_detector.hpp"

#include <algorithm>
#include <cctype>
#include <string_view>

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#else
#  include <dlfcn.h>
#endif

#include "ggml-backend.h"
#include "primitives/utils_logger.hpp"

namespace cantatema::infra {

static std::string to_lowercase(std::string_view str)
{
    std::string result{str};
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

/**
 * @brief Dynamically probe system CUDA driver API for NVIDIA GPU devices.
 */
static bool probe_cuda_driver(AccelerationReport& report)
{
#ifdef _WIN32
    HMODULE hModule = LoadLibraryA("nvcuda.dll");
    if (!hModule) {
        return false;
    }

    typedef int (__stdcall *PFN_cuInit)(unsigned int Flags);
    typedef int (__stdcall *PFN_cuDeviceGetCount)(int *count);
    typedef int (__stdcall *PFN_cuDeviceGet)(int *device, int ordinal);
    typedef int (__stdcall *PFN_cuDeviceGetName)(char *name, int len, int dev);
    typedef int (__stdcall *PFN_cuDeviceTotalMem)(std::size_t *bytes, int dev);

    auto cuInit_fn           = reinterpret_cast<PFN_cuInit>(GetProcAddress(hModule, "cuInit"));
    auto cuDeviceGetCount_fn = reinterpret_cast<PFN_cuDeviceGetCount>(GetProcAddress(hModule, "cuDeviceGetCount"));
    auto cuDeviceGet_fn      = reinterpret_cast<PFN_cuDeviceGet>(GetProcAddress(hModule, "cuDeviceGet"));
    auto cuDeviceGetName_fn  = reinterpret_cast<PFN_cuDeviceGetName>(GetProcAddress(hModule, "cuDeviceGetName"));
    auto cuDeviceTotalMem_fn = reinterpret_cast<PFN_cuDeviceTotalMem>(GetProcAddress(hModule, "cuDeviceTotalMem_v2"));
    if (!cuDeviceTotalMem_fn) {
        cuDeviceTotalMem_fn  = reinterpret_cast<PFN_cuDeviceTotalMem>(GetProcAddress(hModule, "cuDeviceTotalMem"));
    }

    if (!cuInit_fn || !cuDeviceGetCount_fn || cuInit_fn(0) != 0) {
        FreeLibrary(hModule);
        return false;
    }

    int dev_count = 0;
    if (cuDeviceGetCount_fn(&dev_count) != 0 || dev_count <= 0) {
        FreeLibrary(hModule);
        return false;
    }

    report.has_cuda    = true;
    report.has_any_gpu = true;

    for (int i = 0; i < dev_count; ++i) {
        int dev_handle = 0;
        if (cuDeviceGet_fn && cuDeviceGet_fn(&dev_handle, i) == 0) {
            char name_buf[256] = "NVIDIA CUDA GPU";
            if (cuDeviceGetName_fn) {
                cuDeviceGetName_fn(name_buf, sizeof(name_buf), dev_handle);
            }
            std::size_t total_bytes = 0;
            if (cuDeviceTotalMem_fn) {
                cuDeviceTotalMem_fn(&total_bytes, dev_handle);
            }

            DeviceDetails dev{};
            dev.name            = "CUDA" + std::to_string(i);
            dev.description     = name_buf;
            dev.backend_name    = "CUDA Driver";
            dev.type_str        = "GPU (Dedicated)";
            dev.is_gpu          = true;
            dev.memory_free_mb  = total_bytes / (1024 * 1024);
            dev.memory_total_mb = total_bytes / (1024 * 1024);

            logger->info("  [Hardware CUDA Driver] Device: '{}' | Desc: '{}' | VRAM: {} MB",
                         dev.name, dev.description, dev.memory_total_mb);

            report.devices.push_back(dev);
        }
    }

    FreeLibrary(hModule);
    return true;
#else
    void* handle = dlopen("libcuda.so.1", RTLD_NOW);
    if (!handle) {
        handle = dlopen("libcuda.so", RTLD_NOW);
    }
    if (!handle) {
        return false;
    }

    typedef int (*PFN_cuInit)(unsigned int Flags);
    typedef int (*PFN_cuDeviceGetCount)(int *count);
    typedef int (*PFN_cuDeviceGet)(int *device, int ordinal);
    typedef int (*PFN_cuDeviceGetName)(char *name, int len, int dev);
    typedef int (*PFN_cuDeviceTotalMem)(std::size_t *bytes, int dev);

    auto cuInit_fn           = reinterpret_cast<PFN_cuInit>(dlsym(handle, "cuInit"));
    auto cuDeviceGetCount_fn = reinterpret_cast<PFN_cuDeviceGetCount>(dlsym(handle, "cuDeviceGetCount"));
    auto cuDeviceGet_fn      = reinterpret_cast<PFN_cuDeviceGet>(dlsym(handle, "cuDeviceGet"));
    auto cuDeviceGetName_fn  = reinterpret_cast<PFN_cuDeviceGetName>(dlsym(handle, "cuDeviceGetName"));
    auto cuDeviceTotalMem_fn = reinterpret_cast<PFN_cuDeviceTotalMem>(dlsym(handle, "cuDeviceTotalMem_v2"));
    if (!cuDeviceTotalMem_fn) {
        cuDeviceTotalMem_fn  = reinterpret_cast<PFN_cuDeviceTotalMem>(dlsym(handle, "cuDeviceTotalMem"));
    }

    if (!cuInit_fn || !cuDeviceGetCount_fn || cuInit_fn(0) != 0) {
        dlclose(handle);
        return false;
    }

    int dev_count = 0;
    if (cuDeviceGetCount_fn(&dev_count) != 0 || dev_count <= 0) {
        dlclose(handle);
        return false;
    }

    report.has_cuda    = true;
    report.has_any_gpu = true;

    for (int i = 0; i < dev_count; ++i) {
        int dev_handle = 0;
        if (cuDeviceGet_fn && cuDeviceGet_fn(&dev_handle, i) == 0) {
            char name_buf[256] = "NVIDIA CUDA GPU";
            if (cuDeviceGetName_fn) {
                cuDeviceGetName_fn(name_buf, sizeof(name_buf), dev_handle);
            }
            std::size_t total_bytes = 0;
            if (cuDeviceTotalMem_fn) {
                cuDeviceTotalMem_fn(&total_bytes, dev_handle);
            }

            DeviceDetails dev{};
            dev.name            = "CUDA" + std::to_string(i);
            dev.description     = name_buf;
            dev.backend_name    = "CUDA Driver";
            dev.type_str        = "GPU (Dedicated)";
            dev.is_gpu          = true;
            dev.memory_free_mb  = total_bytes / (1024 * 1024);
            dev.memory_total_mb = total_bytes / (1024 * 1024);

            logger->info("  [Hardware CUDA Driver] Device: '{}' | Desc: '{}' | VRAM: {} MB",
                         dev.name, dev.description, dev.memory_total_mb);

            report.devices.push_back(dev);
        }
    }

    dlclose(handle);
    return true;
#endif
}

AccelerationReport GpuDetector::detect_accelerators()
{
    AccelerationReport report{};

    logger->info("Probing host hardware accelerators and registered GGML backends...");

    // Step 1: Probe host system CUDA driver directly (nvcuda.dll / libcuda.so)
    const bool cuda_found = probe_cuda_driver(report);
    if (cuda_found) {
        logger->info("System CUDA Driver: Active NVIDIA GPU hardware detected.");
    } else {
        logger->info("System CUDA Driver: No NVIDIA CUDA driver API responded.");
    }

    // Step 2: Enumerate registered GGML backend devices
    const std::size_t ggml_count = ggml_backend_dev_count();
    logger->info("GGML Backend Registry: Found {} device(s) registered.", ggml_count);

    for (std::size_t i = 0; i < ggml_count; ++i) {
        ggml_backend_dev_t dev = ggml_backend_dev_get(i);
        if (!dev) {
            continue;
        }

        DeviceDetails details{};
        const char*   dev_name = ggml_backend_dev_name(dev);
        const char*   dev_desc = ggml_backend_dev_description(dev);
        const auto    dev_type = ggml_backend_dev_type(dev);

        details.name        = dev_name ? dev_name : "Unknown";
        details.description = dev_desc ? dev_desc : "Unknown";

        ggml_backend_reg_t reg = ggml_backend_dev_backend_reg(dev);
        if (reg) {
            const char* reg_name = ggml_backend_reg_name(reg);
            details.backend_name = reg_name ? reg_name : "Unknown";
        } else {
            details.backend_name = "Unknown";
        }

        std::size_t free_bytes = 0, total_bytes = 0;
        ggml_backend_dev_memory(dev, &free_bytes, &total_bytes);
        details.memory_free_mb  = free_bytes / (1024 * 1024);
        details.memory_total_mb = total_bytes / (1024 * 1024);

        switch (dev_type) {
        case GGML_BACKEND_DEVICE_TYPE_GPU:
            details.type_str = "GPU (Dedicated)";
            details.is_gpu   = true;
            break;
        case GGML_BACKEND_DEVICE_TYPE_IGPU:
            details.type_str = "GPU (Integrated)";
            details.is_gpu   = true;
            break;
        case GGML_BACKEND_DEVICE_TYPE_ACCEL:
            details.type_str = "Accelerator";
            break;
        case GGML_BACKEND_DEVICE_TYPE_CPU:
        default:
            details.type_str = "CPU";
            break;
        }

        const std::string name_lc    = to_lowercase(details.name);
        const std::string desc_lc    = to_lowercase(details.description);
        const std::string backend_lc = to_lowercase(details.backend_name);

        if (name_lc.find("cuda") != std::string::npos ||
            backend_lc.find("cuda") != std::string::npos ||
            desc_lc.find("cuda") != std::string::npos) {
            report.has_cuda = true;
            details.is_gpu  = true;
        }

        if (name_lc.find("vulkan") != std::string::npos ||
            backend_lc.find("vulkan") != std::string::npos ||
            desc_lc.find("vulkan") != std::string::npos) {
            report.has_vulkan = true;
            details.is_gpu    = true;
        }

        if (name_lc.find("metal") != std::string::npos ||
            backend_lc.find("metal") != std::string::npos ||
            desc_lc.find("metal") != std::string::npos) {
            report.has_metal = true;
            details.is_gpu   = true;
        }

        if (details.is_gpu) {
            report.has_any_gpu = true;
        }

        logger->info("  [GGML Device {}] Name: '{}' | Backend: '{}' | Type: {} | Desc: '{}' | Memory: {}/{} MB",
                     i, details.name, details.backend_name, details.type_str,
                     details.description, details.memory_free_mb, details.memory_total_mb);

        report.devices.push_back(details);
    }

    bool ggml_has_cuda   = false;
    bool ggml_has_vulkan = false;
    bool ggml_has_metal  = false;

    for (const auto& dev : report.devices) {
        if (dev.backend_name == "CUDA Driver") {
            continue;
        }

        const std::string name_lc    = to_lowercase(dev.name);
        const std::string desc_lc    = to_lowercase(dev.description);
        const std::string backend_lc = to_lowercase(dev.backend_name);

        if (name_lc.find("cuda") != std::string::npos ||
            backend_lc.find("cuda") != std::string::npos ||
            desc_lc.find("cuda") != std::string::npos) {
            ggml_has_cuda = true;
        }

        if (name_lc.find("vulkan") != std::string::npos ||
            backend_lc.find("vulkan") != std::string::npos ||
            desc_lc.find("vulkan") != std::string::npos) {
            ggml_has_vulkan = true;
        }

        if (name_lc.find("metal") != std::string::npos ||
            backend_lc.find("metal") != std::string::npos ||
            desc_lc.find("metal") != std::string::npos) {
            ggml_has_metal = true;
        }
    }

#if defined(__APPLE__)
    if (ggml_has_metal) {
        report.use_gpu              = true;
        report.selected_device_name = "Metal";
        logger->info("Metal accelerator registered in GGML! Strategy: Metal GPU acceleration (macOS/iOS).");
    } else {
        report.use_gpu              = false;
        report.selected_device_name = "CPU";
        logger->info("No Metal GPU backend registered in GGML. Falling back to CPU backend.");
    }
#else
    if (ggml_has_cuda) {
        report.use_gpu              = true;
        report.selected_device_name = "CUDA";
        logger->info("CUDA accelerator registered in GGML! Strategy: CUDA GPU acceleration (Windows/Linux priority 1).");
    } else if (ggml_has_vulkan) {
        report.use_gpu              = true;
        report.selected_device_name = "Vulkan";
        logger->info("Vulkan accelerator registered in GGML! Strategy: Vulkan GPU acceleration (Windows/Linux priority 2).");
    } else {
        report.use_gpu              = false;
        report.selected_device_name = "CPU";
        if (report.has_cuda) {
            logger->info("Host NVIDIA CUDA GPU hardware detected, but GGML was compiled without CUDA backend support. Falling back to CPU backend.");
        } else {
            logger->info("No active CUDA or Vulkan GPU accelerator found in GGML. Falling back to CPU backend.");
        }
    }
#endif

    return report;
}

AccelerationReport detect_accelerators()
{
    GpuDetector detector;
    return detector.detect_accelerators();
}

} // namespace cantatema::infra
