#include "pch.h"
// ============================================================================
// TreadmillOpenXRLayer - Treadmill Input Handler Implementation
// ============================================================================
#include <cstdarg>
#include <chrono>
#include <algorithm>
#include <cctype>

namespace TreadmillLayer {

// ============================================================================
// GLOBALS
// ============================================================================

TreadmillState g_treadmillState;
Config g_config;

HMODULE OmniBridge::s_library = nullptr;
void* OmniBridge::s_reader = nullptr;
std::atomic<bool> OmniBridge::s_connected{ false };

static std::ofstream g_logFile;
static std::mutex g_logMutex;

// ============================================================================
// LOGGING
// ============================================================================

void InitLogging(const std::wstring& logPath) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    g_logFile.open(logPath, std::ios::out | std::ios::trunc);
    if (g_logFile.is_open()) {
        g_logFile << "========================================\n";
        g_logFile << "TreadmillOpenXRLayer Log\n";
        g_logFile << "========================================\n";
        g_logFile.flush();
    }
}

void ShutdownLogging() {
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (g_logFile.is_open()) {
        g_logFile << "========================================\n";
        g_logFile << "Log closed\n";
        g_logFile.close();
    }
}

void Log(const char* format, ...) {
    if (!g_config.debugLog) return;
    
    std::lock_guard<std::mutex> lock(g_logMutex);
    
    char buffer[2048];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    if (g_logFile.is_open()) {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        char timeBuf[64];
        ctime_s(timeBuf, sizeof(timeBuf), &time);
        timeBuf[strlen(timeBuf) - 1] = '\0';
        
        g_logFile << "[" << timeBuf << "] " << buffer << "\n";
        g_logFile.flush();
    }
    
    OutputDebugStringA("[TreadmillOpenXRLayer] ");
    OutputDebugStringA(buffer);
    OutputDebugStringA("\n");
}

// ============================================================================
// OMNIBRIDGE
// ============================================================================

void OmniBridge::OnOmniData(float ringAngle, int gamePadX, int gamePadY) {
    auto now = std::chrono::steady_clock::now();
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    float x = (static_cast<float>(gamePadX) - 127.0f) / 127.0f;
    float y = -(static_cast<float>(gamePadY) - 127.0f) / 127.0f;
    
    x = ApplyDeadzone(x, g_config.deadzone);
    y = ApplyDeadzone(y, g_config.deadzone);
    
    x *= g_config.speedMultiplier;
    y *= g_config.speedMultiplier;
    
    x = std::clamp(x, -1.0f, 1.0f);
    y = std::clamp(y, -1.0f, 1.0f);
    
    float prevX = g_treadmillState.x.load();
    float prevY = g_treadmillState.y.load();
    
    g_treadmillState.x.store(ApplySmoothing(prevX, x, g_config.smoothing));
    g_treadmillState.y.store(ApplySmoothing(prevY, y, g_config.smoothing));
    g_treadmillState.yaw.store(ringAngle);
    g_treadmillState.lastUpdateTime.store(timestamp);
    g_treadmillState.updateCount.fetch_add(1);
    g_treadmillState.active.store(true);
    
    if (g_treadmillState.updateCount.load() % 100 == 0) {
        Log("Treadmill: X=%.3f Y=%.3f Yaw=%.1f", 
            g_treadmillState.x.load(), g_treadmillState.y.load(), ringAngle);
    }
}

bool OmniBridge::Initialize(const std::wstring& dllPath, const std::string& comPort, int baudRate) {
    Log("Initializing OmniBridge...");
    
    s_library = LoadLibraryW(dllPath.c_str());
    if (!s_library) {
        s_library = LoadLibraryW(L"OmniBridge.dll");
    }
    
    if (!s_library) {
        Log("Failed to load OmniBridge.dll (error %lu)", GetLastError());
        return false;
    }
    
    auto pfnCreate = (PFN_Create)GetProcAddress(s_library, "OmniReader_Create");
    auto pfnInit = (PFN_Initialize)GetProcAddress(s_library, "OmniReader_Initialize");
    auto pfnRegister = (PFN_RegisterCallback)GetProcAddress(s_library, "OmniReader_RegisterCallback");
    
    if (!pfnCreate || !pfnInit || !pfnRegister) {
        Log("Failed to load OmniBridge functions");
        FreeLibrary(s_library);
        s_library = nullptr;
        return false;
    }
    
    s_reader = pfnCreate();
    if (!s_reader) {
        Log("OmniReader_Create failed");
        FreeLibrary(s_library);
        s_library = nullptr;
        return false;
    }
    
    Log("Connecting to treadmill on %s at %d baud...", comPort.c_str(), baudRate);
    
    if (!pfnInit(s_reader, comPort.c_str(), 0, baudRate)) {
        Log("Failed to connect to treadmill");
        auto pfnDestroy = (PFN_Destroy)GetProcAddress(s_library, "OmniReader_Destroy");
        if (pfnDestroy) pfnDestroy(s_reader);
        FreeLibrary(s_library);
        s_library = nullptr;
        s_reader = nullptr;
        return false;
    }
    
    pfnRegister(s_reader, (void*)OnOmniData);
    s_connected.store(true);
    
    Log("Treadmill connected successfully!");
    return true;
}

void OmniBridge::Shutdown() {
    if (s_reader && s_library) {
        auto pfnDisconnect = (PFN_Disconnect)GetProcAddress(s_library, "OmniReader_Disconnect");
        auto pfnDestroy = (PFN_Destroy)GetProcAddress(s_library, "OmniReader_Destroy");
        
        if (pfnDisconnect) pfnDisconnect(s_reader);
        if (pfnDestroy) pfnDestroy(s_reader);
    }
    
    if (s_library) {
        FreeLibrary(s_library);
    }
    
    s_reader = nullptr;
    s_library = nullptr;
    s_connected.store(false);
    
    Log("OmniBridge shut down");
}

bool OmniBridge::IsConnected() {
    return s_connected.load();
}

// ============================================================================
// CONFIGURATION
// ============================================================================

Config Config::Load(const std::wstring& jsonPath) {
    Config config;
    
    std::ifstream file(jsonPath);
    if (!file.is_open()) {
        Log("Config file not found, using defaults");
        return config;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        size_t commentPos = line.find("//");
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }
        
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 1);
            
            auto trim = [](std::string& s) {
                s.erase(0, s.find_first_not_of(" \t\r\n\""));
                s.erase(s.find_last_not_of(" \t\r\n\",") + 1);
            };
            trim(key);
            trim(value);
            
            if (key == "enabled") config.enabled = (value == "true");
            else if (key == "comPort") config.comPort = value;
            else if (key == "baudRate") config.baudRate = std::stoi(value);
            else if (key == "speedMultiplier") config.speedMultiplier = std::stof(value);
            else if (key == "deadzone") config.deadzone = std::stof(value);
            else if (key == "smoothing") config.smoothing = std::stof(value);
            else if (key == "inputMode") {
                if (value == "override") config.inputMode = InputMode::Override;
                else if (value == "additive") config.inputMode = InputMode::Additive;
                else config.inputMode = InputMode::Smart;
            }
            else if (key == "debugLog") config.debugLog = (value == "true");
        }
    }
    
    return config;
}

// ============================================================================
// UTILITY
// ============================================================================

float ApplyDeadzone(float value, float deadzone) {
    if (std::abs(value) < deadzone) {
        return 0.0f;
    }
    float sign = (value > 0.0f) ? 1.0f : -1.0f;
    return sign * (std::abs(value) - deadzone) / (1.0f - deadzone);
}

float ApplySmoothing(float current, float target, float factor) {
    return current + (target - current) * factor;
}

bool MatchesPattern(const std::string& text, const std::string& pattern) {
    std::string lowerText = text;
    std::string lowerPattern = pattern;
    std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::tolower);
    std::transform(lowerPattern.begin(), lowerPattern.end(), lowerPattern.begin(), ::tolower);
    
    if (lowerPattern.empty()) return false;
    
    bool startWild = (lowerPattern[0] == '*');
    bool endWild = (lowerPattern.back() == '*');
    
    std::string searchStr = lowerPattern;
    if (startWild) searchStr = searchStr.substr(1);
    if (endWild && !searchStr.empty()) searchStr = searchStr.substr(0, searchStr.size() - 1);
    
    if (startWild && endWild) {
        return lowerText.find(searchStr) != std::string::npos;
    } else if (startWild) {
        return lowerText.size() >= searchStr.size() && 
               lowerText.substr(lowerText.size() - searchStr.size()) == searchStr;
    } else if (endWild) {
        return lowerText.substr(0, searchStr.size()) == searchStr;
    } else {
        return lowerText == searchStr;
    }
}

} // namespace TreadmillLayer
