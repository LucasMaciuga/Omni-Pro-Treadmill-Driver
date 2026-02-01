#include "pch.h"
// ============================================================================
// TreadmillOpenXRLayer - OpenXR Function Interception Implementation
// ============================================================================
#include <cstring>

using namespace TreadmillLayer;

// ============================================================================
// DISPATCH TABLE
// ============================================================================

static PFN_xrGetInstanceProcAddr g_nextGetInstanceProcAddr = nullptr;

// Real function pointers - use XRAPI_CALL for calling convention
typedef XrResult (XRAPI_CALL *PFN_xrDestroyInstance)(XrInstance);
typedef XrResult (XRAPI_CALL *PFN_xrGetActionStateFloat)(XrSession, const XrActionStateGetInfo*, XrActionStateFloat*);
typedef XrResult (XRAPI_CALL *PFN_xrGetActionStateVector2f)(XrSession, const XrActionStateGetInfo*, XrActionStateVector2f*);
typedef XrResult (XRAPI_CALL *PFN_xrSyncActions)(XrSession, const XrActionsSyncInfo*);
typedef XrResult (XRAPI_CALL *PFN_xrCreateActionSet)(XrInstance, const XrActionSetCreateInfo*, XrActionSet*);
typedef XrResult (XRAPI_CALL *PFN_xrCreateAction)(XrActionSet, const XrActionCreateInfo*, XrAction*);

static PFN_xrDestroyInstance Real_xrDestroyInstance = nullptr;
static PFN_xrGetActionStateFloat Real_xrGetActionStateFloat = nullptr;
static PFN_xrGetActionStateVector2f Real_xrGetActionStateVector2f = nullptr;
static PFN_xrSyncActions Real_xrSyncActions = nullptr;
static PFN_xrCreateActionSet Real_xrCreateActionSet = nullptr;
static PFN_xrCreateAction Real_xrCreateAction = nullptr;

// Action tracking
static std::unordered_map<XrAction, std::string> g_actionNames;
static std::unordered_map<XrAction, bool> g_isMovementAction;

void InitializeDispatchTable(XrInstance instance, PFN_xrGetInstanceProcAddr getInstanceProcAddr) {
    g_nextGetInstanceProcAddr = getInstanceProcAddr;
    
    if (!getInstanceProcAddr) return;
    
    PFN_xrVoidFunction func;
    
    if (XR_SUCCEEDED(getInstanceProcAddr(instance, "xrDestroyInstance", &func))) {
        Real_xrDestroyInstance = (PFN_xrDestroyInstance)func;
    }
    if (XR_SUCCEEDED(getInstanceProcAddr(instance, "xrGetActionStateFloat", &func))) {
        Real_xrGetActionStateFloat = (PFN_xrGetActionStateFloat)func;
    }
    if (XR_SUCCEEDED(getInstanceProcAddr(instance, "xrGetActionStateVector2f", &func))) {
        Real_xrGetActionStateVector2f = (PFN_xrGetActionStateVector2f)func;
    }
    if (XR_SUCCEEDED(getInstanceProcAddr(instance, "xrSyncActions", &func))) {
        Real_xrSyncActions = (PFN_xrSyncActions)func;
    }
    if (XR_SUCCEEDED(getInstanceProcAddr(instance, "xrCreateActionSet", &func))) {
        Real_xrCreateActionSet = (PFN_xrCreateActionSet)func;
    }
    if (XR_SUCCEEDED(getInstanceProcAddr(instance, "xrCreateAction", &func))) {
        Real_xrCreateAction = (PFN_xrCreateAction)func;
    }
    
    Log("Dispatch table initialized");
}

// ============================================================================
// INTERCEPTED FUNCTIONS
// ============================================================================

XrResult XRAPI_CALL TreadmillLayer_xrDestroyInstance(XrInstance instance) {
    Log("xrDestroyInstance called");
    
    // Clear action tracking
    g_actionNames.clear();
    g_isMovementAction.clear();
    
    if (Real_xrDestroyInstance) {
        return Real_xrDestroyInstance(instance);
    }
    return XR_ERROR_HANDLE_INVALID;
}

XrResult XRAPI_CALL TreadmillLayer_xrCreateActionSet(XrInstance instance, const XrActionSetCreateInfo* createInfo, XrActionSet* actionSet) {
    if (!Real_xrCreateActionSet) {
        return XR_ERROR_FUNCTION_UNSUPPORTED;
    }
    
    XrResult result = Real_xrCreateActionSet(instance, createInfo, actionSet);
    
    if (XR_SUCCEEDED(result) && createInfo) {
        Log("ActionSet created: %s", createInfo->actionSetName);
    }
    
    return result;
}

XrResult XRAPI_CALL TreadmillLayer_xrCreateAction(XrActionSet actionSet, const XrActionCreateInfo* createInfo, XrAction* action) {
    if (!Real_xrCreateAction) {
        return XR_ERROR_FUNCTION_UNSUPPORTED;
    }
    
    XrResult result = Real_xrCreateAction(actionSet, createInfo, action);
    
    if (XR_SUCCEEDED(result) && createInfo && action) {
        std::string actionName = createInfo->actionName;
        g_actionNames[*action] = actionName;
        
        // Check if this is a movement action
        bool isMovement = false;
        for (const auto& pattern : g_config.actionPatterns) {
            if (MatchesPattern(actionName, pattern)) {
                isMovement = true;
                break;
            }
        }
        g_isMovementAction[*action] = isMovement;
        
        if (isMovement) {
            Log("Movement action created: %s (type=%d)", actionName.c_str(), createInfo->actionType);
        }
    }
    
    return result;
}

XrResult XRAPI_CALL TreadmillLayer_xrSyncActions(XrSession session, const XrActionsSyncInfo* syncInfo) {
    if (!Real_xrSyncActions) {
        return XR_ERROR_FUNCTION_UNSUPPORTED;
    }
    
    return Real_xrSyncActions(session, syncInfo);
}

XrResult XRAPI_CALL TreadmillLayer_xrGetActionStateFloat(XrSession session, const XrActionStateGetInfo* getInfo, XrActionStateFloat* state) {
    if (!Real_xrGetActionStateFloat || !getInfo || !state) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    XrResult result = Real_xrGetActionStateFloat(session, getInfo, state);
    
    // Check if this is a movement action and inject treadmill data
    if (XR_SUCCEEDED(result) && OmniBridge::IsConnected()) {
        auto it = g_isMovementAction.find(getInfo->action);
        bool isMovement = (it != g_isMovementAction.end() && it->second);
        
        if (isMovement) {
            float treadmillValue = 0.0f;
            
            // Determine which axis based on action name
            auto nameIt = g_actionNames.find(getInfo->action);
            if (nameIt != g_actionNames.end()) {
                std::string name = nameIt->second;
                if (name.find("forward") != std::string::npos || 
                    name.find("vertical") != std::string::npos ||
                    name.find("y") != std::string::npos) {
                    treadmillValue = g_treadmillState.y.load();
                } else {
                    treadmillValue = g_treadmillState.x.load();
                }
            }
            
            bool treadmillActive = std::abs(treadmillValue) > 0.05f;
            
            if (treadmillActive) {
                switch (g_config.inputMode) {
                case Config::InputMode::Override:
                    state->currentState = treadmillValue;
                    state->isActive = true;
                    break;
                    
                case Config::InputMode::Additive:
                    state->currentState = std::clamp(state->currentState + treadmillValue, -1.0f, 1.0f);
                    state->isActive = true;
                    break;
                    
                case Config::InputMode::Smart:
                default:
                    state->currentState = treadmillValue;
                    state->isActive = true;
                    break;
                }
            }
        }
    }
    
    return result;
}

XrResult XRAPI_CALL TreadmillLayer_xrGetActionStateVector2f(XrSession session, const XrActionStateGetInfo* getInfo, XrActionStateVector2f* state) {
    if (!Real_xrGetActionStateVector2f || !getInfo || !state) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    XrResult result = Real_xrGetActionStateVector2f(session, getInfo, state);
    
    // Check if this is a movement action and inject treadmill data
    if (XR_SUCCEEDED(result) && OmniBridge::IsConnected()) {
        auto it = g_isMovementAction.find(getInfo->action);
        bool isMovement = (it != g_isMovementAction.end() && it->second);
        
        if (isMovement) {
            float treadmillX = g_treadmillState.x.load();
            float treadmillY = g_treadmillState.y.load();
            bool treadmillActive = (std::abs(treadmillX) > 0.05f || std::abs(treadmillY) > 0.05f);
            
            if (treadmillActive || g_config.inputMode == Config::InputMode::Additive) {
                switch (g_config.inputMode) {
                case Config::InputMode::Override:
                    state->x = treadmillX;
                    state->y = treadmillY;
                    state->isActive = true;
                    break;
                    
                case Config::InputMode::Additive:
                    state->x = std::clamp(state->x + treadmillX, -1.0f, 1.0f);
                    state->y = std::clamp(state->y + treadmillY, -1.0f, 1.0f);
                    if (treadmillActive) state->isActive = true;
                    break;
                    
                case Config::InputMode::Smart:
                default:
                    if (treadmillActive) {
                        state->x = treadmillX;
                        state->y = treadmillY;
                        state->isActive = true;
                    }
                    break;
                }
                
                // Debug log occasionally
                static uint64_t callCount = 0;
                if (++callCount % 500 == 0 && treadmillActive) {
                    Log("Injected Vector2f: X=%.3f Y=%.3f", state->x, state->y);
                }
            }
        }
    }
    
    return result;
}
