#include "pch.h"
// ============================================================================
// TreadmillOpenVRWrapper - Main DLL Entry and OpenVR API Wrapper
// ============================================================================
// This DLL wraps the OpenVR API to inject treadmill input into VR games.
//
// How it works:
// 1. Game loads this DLL thinking it's openvr_api.dll
// 2. We load the real openvr_api_original.dll
// 3. We forward all calls to the real DLL
// 4. For input-related calls, we inject treadmill data
// ============================================================================

#include "framework.h"
#include "treadmill_input.h"
#include "openvr_wrapper.h"

using namespace TreadmillWrapper;

// ============================================================================
// GLOBALS
// ============================================================================

static HMODULE g_realOpenVR = nullptr;
static HMODULE g_thisModule = nullptr;
static bool g_initialized = false;

// ============================================================================
// INITIALIZATION
// ============================================================================

static std::wstring GetModuleDirectory(HMODULE hModule) {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(hModule, path, MAX_PATH);
    std::wstring fullPath(path);
    size_t lastSlash = fullPath.find_last_of(L"\\/");
    if (lastSlash != std::wstring::npos) {
        return fullPath.substr(0, lastSlash);
    }
    return L".";
}

static bool InitializeWrapper() {
    if (g_initialized) return true;
    
    std::wstring moduleDir = GetModuleDirectory(g_thisModule);
    
    // Initialize logging
    g_config.logPath = moduleDir + L"\\treadmill_wrapper.log";
    InitLogging(g_config.logPath);
    
    LogInfo("TreadmillOpenVRWrapper Initializing");
    LogDebug("Module directory: %ls", moduleDir.c_str());
    
    // Load configuration
    std::wstring configPath = moduleDir + L"\\treadmill_config.json";
    g_config = Config::Load(configPath);
    
    // Configure logger based on config
    Logger::SetDebugEnabled(g_config.debugLog);
    
    LogDebug("Configuration: COM=%s, Speed=%.2f, Mode=%s", 
        g_config.comPort.c_str(), g_config.speedMultiplier,
        g_config.inputMode == Config::InputMode::Override ? "override" :
        g_config.inputMode == Config::InputMode::Additive ? "additive" : "smart");
    
    // Load the real OpenVR DLL
    std::wstring realDllPath = moduleDir + L"\\openvr_api_original.dll";
    g_realOpenVR = LoadLibraryW(realDllPath.c_str());
    
    if (!g_realOpenVR) {
        LogError("FATAL: Could not load openvr_api_original.dll!");
        LogError("Make sure the original DLL is renamed to openvr_api_original.dll");
        return false;
    }
    
    LogDebug("Loaded openvr_api_original.dll");
    
    // Load OpenVR function pointers
    if (!LoadOpenVRFunctions(g_realOpenVR)) {
        LogError("FATAL: Could not load OpenVR functions!");
        return false;
    }
    
    LogDebug("OpenVR functions loaded");
    
    // Initialize treadmill connection
    if (g_config.enabled) {
        std::wstring omniBridgePath = moduleDir + L"\\OmniBridge.dll";
        if (OmniBridge::Initialize(omniBridgePath, g_config.comPort, g_config.baudRate)) {
            LogInfo("Treadmill input active!");
        } else {
            LogInfo("Treadmill not connected - passthrough only");
        }
    } else {
        LogInfo("Treadmill input disabled in config");
    }
    
    g_initialized = true;
    LogInfo("Initialization complete!");
    
    return true;
}

static void ShutdownWrapper() {
    LogInfo("Shutting down wrapper...");
    
    OmniBridge::Shutdown();
    
    if (g_realOpenVR) {
        FreeLibrary(g_realOpenVR);
        g_realOpenVR = nullptr;
    }
    
    ShutdownLogging();
    g_initialized = false;
}

// ============================================================================
// DLL ENTRY POINT
// ============================================================================

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved) {
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        g_thisModule = hModule;
        DisableThreadLibraryCalls(hModule);
        // Don't initialize here - wait for first OpenVR call
        break;
        
    case DLL_PROCESS_DETACH:
        if (reserved == nullptr) {
            ShutdownWrapper();
        }
        break;
    }
    return TRUE;
}

// ============================================================================
// OPENVR INTERFACE GETTERS
// ============================================================================
// These are the main entry points that games call to get OpenVR interfaces

extern "C" {

// VR_InitInternal
__declspec(dllexport) void* VR_CALLTYPE VR_InitInternal(int* peError, int eType) {
    if (!g_initialized) {
        InitializeWrapper();
    }
    
    LogDebug("VR_InitInternal called (type=%d)", eType);
    
    if (Real_VR_InitInternal) {
        return Real_VR_InitInternal(peError, eType);
    }
    
    if (peError) *peError = 110; // VRInitError_Init_InstallationNotFound
    return nullptr;
}

// VR_InitInternal2
__declspec(dllexport) void* VR_CALLTYPE VR_InitInternal2(int* peError, int eType, const char* pStartupInfo) {
    if (!g_initialized) {
        InitializeWrapper();
    }
    
    LogDebug("VR_InitInternal2 called (type=%d)", eType);
    
    if (Real_VR_InitInternal2) {
        return Real_VR_InitInternal2(peError, eType, pStartupInfo);
    }
    
    if (peError) *peError = 110;
    return nullptr;
}

// VR_ShutdownInternal
__declspec(dllexport) void VR_CALLTYPE VR_ShutdownInternal() {
    LogDebug("VR_ShutdownInternal called");
    
    if (Real_VR_ShutdownInternal) {
        Real_VR_ShutdownInternal();
    }
}

// VR_GetVRInitErrorAsEnglishDescription
__declspec(dllexport) const char* VR_CALLTYPE VR_GetVRInitErrorAsEnglishDescription(int error) {
    if (Real_VR_GetVRInitErrorAsEnglishDescription) {
        return Real_VR_GetVRInitErrorAsEnglishDescription(error);
    }
    return "Unknown error";
}

// VR_GetVRInitErrorAsSymbol
__declspec(dllexport) const char* VR_CALLTYPE VR_GetVRInitErrorAsSymbol(int error) {
    if (Real_VR_GetVRInitErrorAsSymbol) {
        return Real_VR_GetVRInitErrorAsSymbol(error);
    }
    return "VRInitError_Unknown";
}

// VR_IsHmdPresent
__declspec(dllexport) bool VR_CALLTYPE VR_IsHmdPresent() {
    if (!g_initialized) {
        InitializeWrapper();
    }
    
    if (Real_VR_IsHmdPresent) {
        return Real_VR_IsHmdPresent();
    }
    return false;
}

// VR_IsRuntimeInstalled
__declspec(dllexport) bool VR_CALLTYPE VR_IsRuntimeInstalled() {
    if (Real_VR_IsRuntimeInstalled) {
        return Real_VR_IsRuntimeInstalled();
    }
    return false;
}

// VR_GetRuntimePath
__declspec(dllexport) bool VR_CALLTYPE VR_GetRuntimePath(char* pchPathBuffer, uint32_t unBufferSize, uint32_t* punRequiredBufferSize) {
    if (Real_VR_GetRuntimePath) {
        return Real_VR_GetRuntimePath(pchPathBuffer, unBufferSize, punRequiredBufferSize);
    }
    return false;
}

// VR_GetStringForHmdError
__declspec(dllexport) const char* VR_CALLTYPE VR_GetStringForHmdError(int error) {
    if (Real_VR_GetStringForHmdError) {
        return Real_VR_GetStringForHmdError(error);
    }
    return "Unknown error";
}

// VR_GetGenericInterface
__declspec(dllexport) void* VR_CALLTYPE VR_GetGenericInterface(const char* pchInterfaceVersion, int* peError) {
    if (!g_initialized) {
        InitializeWrapper();
    }
    
    LogDebug("VR_GetGenericInterface: %s", pchInterfaceVersion);
    
    
    if (Real_VR_GetGenericInterface) {
        void* iface = Real_VR_GetGenericInterface(pchInterfaceVersion, peError);
        
        // Wrap IVRSystem interface to inject treadmill data into legacy input
        if (iface && pchInterfaceVersion && strstr(pchInterfaceVersion, "IVRSystem")) {
            LogDebug("Wrapping IVRSystem interface (legacy input)");
            return WrapIVRSystem(iface);
        }
        
        // Wrap IVRInput interface to inject treadmill data (for modern games)
        if (iface && pchInterfaceVersion && strstr(pchInterfaceVersion, "IVRInput")) {
            LogDebug("Wrapping IVRInput interface");
            return WrapIVRInput(iface);
        }
        
        return iface;
    }
    
    if (peError) *peError = 110;
    return nullptr;
}

// VR_IsInterfaceVersionValid
__declspec(dllexport) bool VR_CALLTYPE VR_IsInterfaceVersionValid(const char* pchInterfaceVersion) {
    if (Real_VR_IsInterfaceVersionValid) {
        return Real_VR_IsInterfaceVersionValid(pchInterfaceVersion);
    }
    return false;
}

// VR_GetInitToken
__declspec(dllexport) uint32_t VR_CALLTYPE VR_GetInitToken() {
    if (Real_VR_GetInitToken) {
        return Real_VR_GetInitToken();
    }
    return 0;
}

} // extern "C"
