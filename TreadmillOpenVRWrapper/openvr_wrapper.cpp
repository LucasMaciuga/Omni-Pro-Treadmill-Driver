#include "pch.h"
// ============================================================================
// TreadmillOpenVRWrapper - OpenVR Function Pointers and IVRInput Wrapper Impl
// ============================================================================
#include <cstring>

using namespace TreadmillWrapper;

// ============================================================================
// REAL FUNCTION POINTERS
// ============================================================================

PFN_VR_InitInternal Real_VR_InitInternal = nullptr;
PFN_VR_InitInternal2 Real_VR_InitInternal2 = nullptr;
PFN_VR_ShutdownInternal Real_VR_ShutdownInternal = nullptr;
PFN_VR_GetVRInitErrorAsEnglishDescription Real_VR_GetVRInitErrorAsEnglishDescription = nullptr;
PFN_VR_GetVRInitErrorAsSymbol Real_VR_GetVRInitErrorAsSymbol = nullptr;
PFN_VR_IsHmdPresent Real_VR_IsHmdPresent = nullptr;
PFN_VR_IsRuntimeInstalled Real_VR_IsRuntimeInstalled = nullptr;
PFN_VR_GetRuntimePath Real_VR_GetRuntimePath = nullptr;
PFN_VR_GetStringForHmdError Real_VR_GetStringForHmdError = nullptr;
PFN_VR_GetGenericInterface Real_VR_GetGenericInterface = nullptr;
PFN_VR_IsInterfaceVersionValid Real_VR_IsInterfaceVersionValid = nullptr;
PFN_VR_GetInitToken Real_VR_GetInitToken = nullptr;

bool LoadOpenVRFunctions(HMODULE realDll) {
    if (!realDll) return false;
    
    Real_VR_InitInternal = (PFN_VR_InitInternal)GetProcAddress(realDll, "VR_InitInternal");
    Real_VR_InitInternal2 = (PFN_VR_InitInternal2)GetProcAddress(realDll, "VR_InitInternal2");
    Real_VR_ShutdownInternal = (PFN_VR_ShutdownInternal)GetProcAddress(realDll, "VR_ShutdownInternal");
    Real_VR_GetVRInitErrorAsEnglishDescription = (PFN_VR_GetVRInitErrorAsEnglishDescription)GetProcAddress(realDll, "VR_GetVRInitErrorAsEnglishDescription");
    Real_VR_GetVRInitErrorAsSymbol = (PFN_VR_GetVRInitErrorAsSymbol)GetProcAddress(realDll, "VR_GetVRInitErrorAsSymbol");
    Real_VR_IsHmdPresent = (PFN_VR_IsHmdPresent)GetProcAddress(realDll, "VR_IsHmdPresent");
    Real_VR_IsRuntimeInstalled = (PFN_VR_IsRuntimeInstalled)GetProcAddress(realDll, "VR_IsRuntimeInstalled");
    Real_VR_GetRuntimePath = (PFN_VR_GetRuntimePath)GetProcAddress(realDll, "VR_GetRuntimePath");
    Real_VR_GetStringForHmdError = (PFN_VR_GetStringForHmdError)GetProcAddress(realDll, "VR_GetStringForHmdError");
    Real_VR_GetGenericInterface = (PFN_VR_GetGenericInterface)GetProcAddress(realDll, "VR_GetGenericInterface");
    Real_VR_IsInterfaceVersionValid = (PFN_VR_IsInterfaceVersionValid)GetProcAddress(realDll, "VR_IsInterfaceVersionValid");
    Real_VR_GetInitToken = (PFN_VR_GetInitToken)GetProcAddress(realDll, "VR_GetInitToken");
    
    // At minimum we need VR_GetGenericInterface
    return Real_VR_GetGenericInterface != nullptr;
}

// ============================================================================
// IVRINPUT WRAPPER
// ============================================================================

// Store the real IVRInput interface and action name mappings
static void* g_realIVRInput = nullptr;
static std::unordered_map<VRActionHandle_t, std::string> g_actionNames;
static std::unordered_map<VRActionHandle_t, bool> g_isMovementAction;

// IVRInput vtable function types
typedef EVRInputError (*PFN_GetActionHandle)(void* self, const char* pchActionName, VRActionHandle_t* pHandle);
typedef EVRInputError (*PFN_GetAnalogActionData)(void* self, VRActionHandle_t action, InputAnalogActionData_t* pActionData, uint32_t unActionDataSize, VRInputValueHandle_t ulRestrictToDevice);

// Our wrapped functions
static EVRInputError Wrapped_GetActionHandle(void* self, const char* pchActionName, VRActionHandle_t* pHandle) {
    // Get real vtable
    void** vtable = *(void***)g_realIVRInput;
    auto realFunc = (PFN_GetActionHandle)vtable[IVRInputVTable::GetActionHandle];
    
    EVRInputError result = realFunc(g_realIVRInput, pchActionName, pHandle);
    
    if (result == VRInputError_None && pHandle && pchActionName) {
        // Store action name for later lookup
        g_actionNames[*pHandle] = pchActionName;
        
        // Check if this is a movement action
        bool isMovement = false;
        for (const auto& pattern : g_config.actionPatterns) {
            if (MatchesPattern(pchActionName, pattern)) {
                isMovement = true;
                break;
            }
        }
        g_isMovementAction[*pHandle] = isMovement;
        
        if (isMovement) {
            LogDebug("Detected movement action: %s (handle=0x%llX)", pchActionName, *pHandle);
        }
    }
    
    return result;
}

static EVRInputError Wrapped_GetAnalogActionData(void* self, VRActionHandle_t action, InputAnalogActionData_t* pActionData, uint32_t unActionDataSize, VRInputValueHandle_t ulRestrictToDevice) {
    // Get real vtable
    void** vtable = *(void***)g_realIVRInput;
    auto realFunc = (PFN_GetAnalogActionData)vtable[IVRInputVTable::GetAnalogActionData];
    
    // Call real function first
    EVRInputError result = realFunc(g_realIVRInput, action, pActionData, unActionDataSize, ulRestrictToDevice);
    
    // Inject treadmill data if this is a movement action
    if (result == VRInputError_None && pActionData) {
        auto it = g_isMovementAction.find(action);
        bool isMovement = (it != g_isMovementAction.end() && it->second);
        
        if (isMovement && OmniBridge::IsConnected()) {
            float treadmillX = g_treadmillState.x.load();
            float treadmillY = g_treadmillState.y.load();
            bool treadmillActive = (std::abs(treadmillX) > 0.05f || std::abs(treadmillY) > 0.05f);
            
            switch (g_config.inputMode) {
            case Config::InputMode::Override:
                if (treadmillActive) {
                    pActionData->x = treadmillX;
                    pActionData->y = treadmillY;
                    pActionData->bActive = true;
                }
                break;
                
            case Config::InputMode::Additive:
                pActionData->x = std::clamp(pActionData->x + treadmillX, -1.0f, 1.0f);
                pActionData->y = std::clamp(pActionData->y + treadmillY, -1.0f, 1.0f);
                if (treadmillActive) pActionData->bActive = true;
                break;
                
            case Config::InputMode::Smart:
            default:
                // Override only when treadmill is active
                if (treadmillActive) {
                    pActionData->x = treadmillX;
                    pActionData->y = treadmillY;
                    pActionData->bActive = true;
                }
                break;
            }
            
            // Debug log occasionally
            static uint64_t callCount = 0;
            if (++callCount % 500 == 0 && treadmillActive) {
                LogTrace("Injected treadmill into action 0x%llX: X=%.3f Y=%.3f", 
                    action, pActionData->x, pActionData->y);
            }
        }
    }
    
    return result;
}

// ============================================================================
// IVRSYSTEM WRAPPER (LEGACY INPUT)
// ============================================================================

static void* g_realIVRSystem = nullptr;

// Function types for IVRSystem
typedef bool (*PFN_GetControllerState)(void* self, TrackedDeviceIndex_t unControllerDeviceIndex, VRControllerState_t* pControllerState, uint32_t unControllerStateSize);
typedef bool (*PFN_GetControllerStateWithPose)(void* self, int eOrigin, TrackedDeviceIndex_t unControllerDeviceIndex, VRControllerState_t* pControllerState, uint32_t unControllerStateSize, void* pTrackedDevicePose);

// Wrapped GetControllerState - injects treadmill input
static bool Wrapped_GetControllerState(void* self, TrackedDeviceIndex_t unControllerDeviceIndex, VRControllerState_t* pControllerState, uint32_t unControllerStateSize) {
    void** vtable = *(void***)g_realIVRSystem;
    auto realFunc = (PFN_GetControllerState)vtable[IVRSystemVTable::GetControllerState];
    
    bool result = realFunc(g_realIVRSystem, unControllerDeviceIndex, pControllerState, unControllerStateSize);
    
    if (result && pControllerState && OmniBridge::IsConnected()) {
        // Filter by target controller if configured
        // -1 = inject into all controllers (legacy behavior, causes jump on right controller!)
        // Specific index = only inject into that controller (recommended: set to left controller)
        if (g_config.targetControllerIndex >= 0 && 
            static_cast<int>(unControllerDeviceIndex) != g_config.targetControllerIndex) {
            return result;  // Skip injection for non-target controllers
        }
        
        float treadmillX = g_treadmillState.x.load();
        float treadmillY = g_treadmillState.y.load();
        bool treadmillActive = (std::abs(treadmillX) > 0.05f || std::abs(treadmillY) > 0.05f);
        
        if (treadmillActive) {
            switch (g_config.inputMode) {
            case Config::InputMode::Override:
                pControllerState->rAxis[k_EControllerAxis_Joystick].x = treadmillX;
                pControllerState->rAxis[k_EControllerAxis_Joystick].y = treadmillY;
                break;
                
            case Config::InputMode::Additive:
                pControllerState->rAxis[k_EControllerAxis_Joystick].x = 
                    std::clamp(pControllerState->rAxis[k_EControllerAxis_Joystick].x + treadmillX, -1.0f, 1.0f);
                pControllerState->rAxis[k_EControllerAxis_Joystick].y = 
                    std::clamp(pControllerState->rAxis[k_EControllerAxis_Joystick].y + treadmillY, -1.0f, 1.0f);
                break;
                
            case Config::InputMode::Smart:
            default:
                pControllerState->rAxis[k_EControllerAxis_Joystick].x = treadmillX;
                pControllerState->rAxis[k_EControllerAxis_Joystick].y = treadmillY;
                break;
            }
            
            // Debug log occasionally
            static uint64_t callCount = 0;
            if (++callCount % 500 == 0) {
                LogTrace("Injected into GetControllerState (device %u): X=%.3f Y=%.3f", 
                    unControllerDeviceIndex, treadmillX, treadmillY);
            }
        }
    }
    
    return result;
}

static bool Wrapped_GetControllerStateWithPose(void* self, int eOrigin, TrackedDeviceIndex_t unControllerDeviceIndex, VRControllerState_t* pControllerState, uint32_t unControllerStateSize, void* pTrackedDevicePose) {
    void** vtable = *(void***)g_realIVRSystem;
    auto realFunc = (PFN_GetControllerStateWithPose)vtable[IVRSystemVTable::GetControllerStateWithPose];
    
    bool result = realFunc(g_realIVRSystem, eOrigin, unControllerDeviceIndex, pControllerState, unControllerStateSize, pTrackedDevicePose);
    
    if (result && pControllerState && OmniBridge::IsConnected()) {
        // Filter by target controller if configured
        if (g_config.targetControllerIndex >= 0 && 
            static_cast<int>(unControllerDeviceIndex) != g_config.targetControllerIndex) {
            return result;  // Skip injection for non-target controllers
        }
        
        float treadmillX = g_treadmillState.x.load();
        float treadmillY = g_treadmillState.y.load();
        bool treadmillActive = (std::abs(treadmillX) > 0.05f || std::abs(treadmillY) > 0.05f);
        
        if (treadmillActive) {
            switch (g_config.inputMode) {
            case Config::InputMode::Override:
                pControllerState->rAxis[k_EControllerAxis_Joystick].x = treadmillX;
                pControllerState->rAxis[k_EControllerAxis_Joystick].y = treadmillY;
                break;
                
            case Config::InputMode::Additive:
                pControllerState->rAxis[k_EControllerAxis_Joystick].x = 
                    std::clamp(pControllerState->rAxis[k_EControllerAxis_Joystick].x + treadmillX, -1.0f, 1.0f);
                pControllerState->rAxis[k_EControllerAxis_Joystick].y = 
                    std::clamp(pControllerState->rAxis[k_EControllerAxis_Joystick].y + treadmillY, -1.0f, 1.0f);
                break;
                
            case Config::InputMode::Smart:
            default:
                pControllerState->rAxis[k_EControllerAxis_Joystick].x = treadmillX;
                pControllerState->rAxis[k_EControllerAxis_Joystick].y = treadmillY;
                break;
            }
            
            static uint64_t callCount = 0;
            if (++callCount % 500 == 0) {
                LogTrace("Injected into GetControllerStateWithPose (device %u): X=%.3f Y=%.3f", 
                    unControllerDeviceIndex, treadmillX, treadmillY);
            }
        }
    }
    
    return result;
}

// ============================================================================
// VTABLE HOOKING
// ============================================================================

// Our wrapper vtables
static void* g_wrappedInputVTable[64] = { nullptr };
static void* g_inputWrapperInstance = nullptr;

static void* g_wrappedSystemVTable[128] = { nullptr };  // IVRSystem has more functions
static void* g_systemWrapperInstance = nullptr;

void* WrapIVRInput(void* realInterface) {
    if (!realInterface) return nullptr;
    
    g_realIVRInput = realInterface;
    
    // Copy the real vtable
    void** realVTable = *(void***)realInterface;
    memcpy(g_wrappedInputVTable, realVTable, sizeof(g_wrappedInputVTable));
    
    // Replace functions we want to intercept
    g_wrappedInputVTable[IVRInputVTable::GetActionHandle] = (void*)Wrapped_GetActionHandle;
    g_wrappedInputVTable[IVRInputVTable::GetAnalogActionData] = (void*)Wrapped_GetAnalogActionData;
    
    // Create a fake object that points to our vtable
    static void* wrappedVTablePtr = g_wrappedInputVTable;
    g_inputWrapperInstance = &wrappedVTablePtr;
    
    LogInfo("IVRInput wrapper created");
    
    return g_inputWrapperInstance;
}

void* WrapIVRSystem(void* realInterface) {
    if (!realInterface) return nullptr;
    
    g_realIVRSystem = realInterface;
    
    // Copy the real vtable
    void** realVTable = *(void***)realInterface;
    memcpy(g_wrappedSystemVTable, realVTable, sizeof(g_wrappedSystemVTable));
    
    // Replace controller state functions
    g_wrappedSystemVTable[IVRSystemVTable::GetControllerState] = (void*)Wrapped_GetControllerState;
    g_wrappedSystemVTable[IVRSystemVTable::GetControllerStateWithPose] = (void*)Wrapped_GetControllerStateWithPose;
    
    // Create a fake object that points to our vtable
    static void* wrappedVTablePtr = g_wrappedSystemVTable;
    g_systemWrapperInstance = &wrappedVTablePtr;
    
    LogInfo("IVRSystem wrapper created (legacy input interception enabled)");
    
    return g_systemWrapperInstance;
}
