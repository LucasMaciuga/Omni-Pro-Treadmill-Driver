// ============================================================================
// TreadmillOpenVRWrapper - Treadmill Input Handler
// ============================================================================
#pragma once

#include "framework.h"

namespace TreadmillWrapper {

// ============================================================================
// TREADMILL STATE
// ============================================================================

struct TreadmillState {
    // Raw values from hardware (normalized -1 to 1)
    std::atomic<float> x{ 0.0f };
    std::atomic<float> y{ 0.0f };
    std::atomic<float> yaw{ 0.0f };
    
    // Is the treadmill active?
    std::atomic<bool> active{ false };
    std::atomic<uint64_t> lastUpdateTime{ 0 };
    std::atomic<uint64_t> updateCount{ 0 };
};

extern TreadmillState g_treadmillState;

// ============================================================================
// OMNIBRIDGE INTERFACE
// ============================================================================

class OmniBridge {
public:
    static bool Initialize(const std::wstring& dllPath, const std::string& comPort, int baudRate);
    static void Shutdown();
    static bool IsConnected();
    
private:
    // OmniBridge function types
    typedef void* (*PFN_Create)();
    typedef bool (*PFN_Initialize)(void*, const char*, int, int);
    typedef void (*PFN_RegisterCallback)(void*, void*);
    typedef void (*PFN_Disconnect)(void*);
    typedef void (*PFN_Destroy)(void*);
    
    static HMODULE s_library;
    static void* s_reader;
    static std::atomic<bool> s_connected;
    
    // Callback from OmniBridge
    static void OnOmniData(float ringAngle, int gamePadX, int gamePadY);
};

// ============================================================================
// CONFIGURATION
// ============================================================================

struct Config {
    bool enabled = true;
    std::string comPort = "COM3";
    int baudRate = 115200;
    float speedMultiplier = 1.5f;
    float deadzone = 0.1f;
    float smoothing = 0.3f;
    
    // Target controller for input injection (-1 = all controllers, specific index = only that controller)
    // For Oculus: Left controller is typically index 1 or 3, Right is 2 or 4
    // Set to left controller index to prevent jump on right controller
    int targetControllerIndex = -1;  // -1 = inject into all (legacy behavior)
    
    enum class InputMode {
        Override,   // Replace controller input
        Additive,   // Add to controller input
        Smart       // Override only when treadmill is active
    };
    InputMode inputMode = InputMode::Smart;
    
    std::vector<std::string> actionPatterns = {
        "*move*", "*locomotion*", "*walk*", "*thumbstick*"
    };
    
    bool debugLog = true;
    std::wstring logPath;
    
    static Config Load(const std::wstring& jsonPath);
};

extern Config g_config;

// ============================================================================
// LOGGING
// ============================================================================

// Logger is now in Logger.h - include it for logging functions

// ============================================================================
// UTILITY
// ============================================================================

float ApplyDeadzone(float value, float deadzone);
float ApplySmoothing(float current, float target, float factor);
bool MatchesPattern(const std::string& text, const std::string& pattern);

} // namespace TreadmillWrapper
