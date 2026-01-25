#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <iostream>
#include <vector>
#include <atomic>
#include <thread>
#include <chrono>
#include <csignal>
#include <cmath> 

// --- Global Control ---
std::atomic<bool> g_isRecording{true};

void signalHandler(int signal) {
    if (signal == SIGINT) g_isRecording = false;
}

// --- Data Callback ---
void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    (void)pOutput;
    
    // 1. Validation
    if (pInput == nullptr) return;
    ma_encoder* pEncoder = (ma_encoder*)pDevice->pUserData;
    if (pEncoder == nullptr) return;

    // 2. Debugging: Check for signal presence
    // If you see "Signal!" in the console, audio is reaching the app.
    // If you don't, the OS is sending silence.
    const float* fInput = static_cast<const float*>(pInput);
    float maxAmp = 0.0f;
    // Check first few samples to see if there is life
    for(ma_uint32 i=0; i < frameCount; ++i) {
        float val = std::abs(fInput[i]);
        if(val > maxAmp) maxAmp = val;
    }
    if (maxAmp > 0.01f) {
        // Uncomment to see visual feedback in terminal
        // std::cout << "Signal Detected: " << maxAmp << "\r" << std::flush; 
    }

    // 3. Write to WAV
    ma_encoder_write_pcm_frames(pEncoder, pInput, frameCount, nullptr);
}

int main() {
    std::signal(SIGINT, signalHandler);

    ma_context context;
    ma_device_info* pCaptureInfos;
    ma_uint32 captureCount;
    ma_device_info* pPlaybackInfos;
    ma_uint32 playbackCount;

    // --- 1. Initialize Context (Required to list devices) ---
    if (ma_context_init(NULL, 0, NULL, &context) != MA_SUCCESS) {
        std::cerr << "Failed to initialize context." << std::endl;
        return -1;
    }

    // --- 2. Enumerate Devices ---
    if (ma_context_get_devices(&context, &pPlaybackInfos, &playbackCount, &pCaptureInfos, &captureCount) != MA_SUCCESS) {
        std::cerr << "Failed to retrieve device information." << std::endl;
        ma_context_uninit(&context);
        return -1;
    }

    std::cout << "--- Available Capture Devices ---" << std::endl;
    for (ma_uint32 i = 0; i < captureCount; ++i) {
        std::cout << i << ": " << pCaptureInfos[i].name;
        if (pCaptureInfos[i].isDefault) std::cout << " [Default]";
        std::cout << std::endl;
    }
    std::cout << "---------------------------------" << std::endl;

    if (captureCount == 0) {
        std::cerr << "No microphone found!" << std::endl;
        return -1;
    }

    // --- 3. Select Device ---
    // Change this index manually if the default (0) is not your mic
    unsigned int selectedDeviceIndex = 2; 
    
    // Optional: Uncomment to let user choose at runtime
    // std::cout << "Enter device index to record from: ";
    // std::cin >> selectedDeviceIndex;

    if (selectedDeviceIndex >= captureCount) {
        std::cerr << "Invalid device index." << std::endl;
        return -1;
    }

    std::cout << "Selected: " << pCaptureInfos[selectedDeviceIndex].name << std::endl;

    // --- 4. Configure Encoder (WAV File) ---
    ma_encoder_config encoderConfig = ma_encoder_config_init(ma_encoding_format_wav, ma_format_f32, 1, 44100);
    ma_encoder encoder;
    if (ma_encoder_init_file("output.wav", &encoderConfig, &encoder) != MA_SUCCESS) {
        std::cerr << "Failed to initialize output file." << std::endl;
        return -1;
    }

    // --- 5. Configure Capture Device ---
    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_capture);
    // Crucial: Tell miniaudio exactly which hardware ID to use
    deviceConfig.capture.pDeviceID = &pCaptureInfos[selectedDeviceIndex].id;
    deviceConfig.capture.format    = ma_format_f32;
    deviceConfig.capture.channels  = 1;
    deviceConfig.sampleRate        = 44100;
    deviceConfig.dataCallback      = data_callback;
    deviceConfig.pUserData         = &encoder;

    ma_device device;
    if (ma_device_init(&context, &deviceConfig, &device) != MA_SUCCESS) {
        std::cerr << "Failed to initialize capture device." << std::endl;
        return -1;
    }

    // --- 6. Start Recording ---
    if (ma_device_start(&device) != MA_SUCCESS) {
        std::cerr << "Failed to start device." << std::endl;
        return -1;
    }

    std::cout << "Recording... Press Ctrl+C to stop." << std::endl;

    while (g_isRecording) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // --- 7. Cleanup ---
    ma_device_uninit(&device);
    ma_encoder_uninit(&encoder); // Finalizes WAV header
    ma_context_uninit(&context);

    std::cout << "Done. Saved to 'output.wav'." << std::endl;
    return 0;
}