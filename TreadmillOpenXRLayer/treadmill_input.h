// ============================================================================
// TreadmillOpenXRLayer - Treadmill Input Handler
// ============================================================================
#pragma once

#include "framework.h"

namespace TreadmillLayer {

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
    typedef void* (*PFN_Create)();
    typedef bool (*PFN_Initialize)(void*, const char*, int, int);
    typedef void (*PFN_RegisterCallback)(void*, void*);
    typedef void (*PFN_Disconnect)(void*);
    typedef void (*PFN_Destroy)(void*);
    
    static HMODULE s_library;
    static void* s_reader;
    static std::atomic<bool> s_connected;
    
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
    
    enum class InputMode {
        Override,
        Additive,
        Smart
    };
    InputMode inputMode = InputMode::Smart;
    
    std::vector<std::string> actionPatterns = {
        "*move*", "*locomotion*", "*walk*", "*thumbstick*"
    };
    
    // OpenXR-specific
    std::vector<std::string> targetPaths = {
        "/user/hand/left/input/thumbstick"
    };
    
    bool debugLog = true;
    std::wstring logPath;
    
    static Config Load(const std::wstring& jsonPath);
};

extern Config g_config;

// ============================================================================
// LOGGING
// ============================================================================

void Log(const char* format, ...);
void InitLogging(const std::wstring& logPath);
void ShutdownLogging();

// ============================================================================
// UTILITY
// ============================================================================

float ApplyDeadzone(float value, float deadzone);
float ApplySmoothing(float current, float target, float factor);
bool MatchesPattern(const std::string& text, const std::string& pattern);

} // namespace TreadmillLayer
