// ============================================================================
// TreadmillOpenVRWrapper - OpenVR Function Pointers and IVRInput Wrapper
// ============================================================================
#pragma once

#include "framework.h"

// ============================================================================
// OPENVR TYPES (Minimal definitions to avoid full SDK dependency)
// ============================================================================

typedef uint64_t VRActionHandle_t;
typedef uint64_t VRInputValueHandle_t;
typedef uint64_t VRActionSetHandle_t;

#define VR_CALLTYPE __cdecl

// InputAnalogActionData_t
struct InputAnalogActionData_t {
    bool bActive;
    VRInputValueHandle_t activeOrigin;
    float x;
    float y;
    float z;
    float deltaX;
    float deltaY;
    float deltaZ;
    float fUpdateTime;
};

// InputDigitalActionData_t
struct InputDigitalActionData_t {
    bool bActive;
    VRInputValueHandle_t activeOrigin;
    bool bState;
    bool bChanged;
    float fUpdateTime;
};

// VRActiveActionSet_t
struct VRActiveActionSet_t {
    VRActionSetHandle_t ulActionSet;
    VRInputValueHandle_t ulRestrictedToDevice;
    VRActionSetHandle_t ulSecondaryActionSet;
    uint32_t unPadding;
    int32_t nPriority;
};

// EVRInputError
enum EVRInputError {
    VRInputError_None = 0,
    VRInputError_NameNotFound = 1,
    VRInputError_WrongType = 2,
    VRInputError_InvalidHandle = 3,
    VRInputError_InvalidParam = 4,
    VRInputError_NoSteam = 5,
    VRInputError_MaxCapacityReached = 6,
    VRInputError_IPCError = 7,
    VRInputError_NoActiveActionSet = 8,
    VRInputError_InvalidDevice = 9,
    VRInputError_InvalidSkeleton = 10,
    VRInputError_InvalidBoneCount = 11,
    VRInputError_InvalidCompressedData = 12,
    VRInputError_NoData = 13,
    VRInputError_BufferTooSmall = 14,
    VRInputError_MismatchedActionManifest = 15,
    VRInputError_MissingSkeletonData = 16,
    VRInputError_InvalidBoneIndex = 17
};

// ============================================================================
// REAL OPENVR FUNCTION POINTERS
// ============================================================================

// VR_Init/Shutdown functions
typedef void* (VR_CALLTYPE *PFN_VR_InitInternal)(int*, int);
typedef void* (VR_CALLTYPE *PFN_VR_InitInternal2)(int*, int, const char*);
typedef void (VR_CALLTYPE *PFN_VR_ShutdownInternal)();
typedef const char* (VR_CALLTYPE *PFN_VR_GetVRInitErrorAsEnglishDescription)(int);
typedef const char* (VR_CALLTYPE *PFN_VR_GetVRInitErrorAsSymbol)(int);
typedef bool (VR_CALLTYPE *PFN_VR_IsHmdPresent)();
typedef bool (VR_CALLTYPE *PFN_VR_IsRuntimeInstalled)();
typedef bool (VR_CALLTYPE *PFN_VR_GetRuntimePath)(char*, uint32_t, uint32_t*);
typedef const char* (VR_CALLTYPE *PFN_VR_GetStringForHmdError)(int);
typedef void* (VR_CALLTYPE *PFN_VR_GetGenericInterface)(const char*, int*);
typedef bool (VR_CALLTYPE *PFN_VR_IsInterfaceVersionValid)(const char*);
typedef uint32_t (VR_CALLTYPE *PFN_VR_GetInitToken)();

// Function pointer globals
extern PFN_VR_InitInternal Real_VR_InitInternal;
extern PFN_VR_InitInternal2 Real_VR_InitInternal2;
extern PFN_VR_ShutdownInternal Real_VR_ShutdownInternal;
extern PFN_VR_GetVRInitErrorAsEnglishDescription Real_VR_GetVRInitErrorAsEnglishDescription;
extern PFN_VR_GetVRInitErrorAsSymbol Real_VR_GetVRInitErrorAsSymbol;
extern PFN_VR_IsHmdPresent Real_VR_IsHmdPresent;
extern PFN_VR_IsRuntimeInstalled Real_VR_IsRuntimeInstalled;
extern PFN_VR_GetRuntimePath Real_VR_GetRuntimePath;
extern PFN_VR_GetStringForHmdError Real_VR_GetStringForHmdError;
extern PFN_VR_GetGenericInterface Real_VR_GetGenericInterface;
extern PFN_VR_IsInterfaceVersionValid Real_VR_IsInterfaceVersionValid;
extern PFN_VR_GetInitToken Real_VR_GetInitToken;

// Load all function pointers from the real DLL
bool LoadOpenVRFunctions(HMODULE realDll);

// ============================================================================
// LEGACY CONTROLLER STATE (for older games like SkyrimVR)
// ============================================================================

// VRControllerState_t - used by IVRSystem::GetControllerState()
#pragma pack(push, 8)
struct VRControllerState001_t {
    uint32_t unPacketNum;
    uint64_t ulButtonPressed;
    uint64_t ulButtonTouched;
    struct {
        float x;
        float y;
    } rAxis[5];  // k_unControllerStateAxisCount = 5
};
#pragma pack(pop)

typedef VRControllerState001_t VRControllerState_t;

// Button masks
#define k_EButton_SteamVR_Touchpad  32
#define k_EButton_Axis0             32
#define k_EButton_Axis1             33

// Axis indices
#define k_EControllerAxis_Joystick  0  // Primary joystick/thumbstick
#define k_EControllerAxis_Trigger   1
#define k_EControllerAxis_TrackPad  2

// TrackedDeviceIndex
typedef uint32_t TrackedDeviceIndex_t;
#define k_unTrackedDeviceIndex_Hmd          0
#define k_unTrackedDeviceIndexInvalid       0xFFFFFFFF

// ============================================================================
// IVRSYSTEM WRAPPER
// ============================================================================

// Wrap the IVRSystem interface to inject treadmill data into legacy input
void* WrapIVRSystem(void* realInterface);

// IVRSystem virtual function indices (from OpenVR SDK)
namespace IVRSystemVTable {
    enum {
        GetRecommendedRenderTargetSize = 0,
        GetProjectionMatrix = 1,
        GetProjectionRaw = 2,
        ComputeDistortion = 3,
        GetEyeToHeadTransform = 4,
        GetTimeSinceLastVsync = 5,
        GetD3D9AdapterIndex = 6,
        GetDXGIOutputInfo = 7,
        GetOutputDevice = 8,
        IsDisplayOnDesktop = 9,
        SetDisplayVisibility = 10,
        GetDeviceToAbsoluteTrackingPose = 11,
        ResetSeatedZeroPose = 12,
        GetSeatedZeroPoseToStandingAbsoluteTrackingPose = 13,
        GetRawZeroPoseToStandingAbsoluteTrackingPose = 14,
        GetSortedTrackedDeviceIndicesOfClass = 15,
        GetTrackedDeviceActivityLevel = 16,
        ApplyTransform = 17,
        GetTrackedDeviceIndexForControllerRole = 18,
        GetControllerRoleForTrackedDeviceIndex = 19,
        GetTrackedDeviceClass = 20,
        IsTrackedDeviceConnected = 21,
        GetBoolTrackedDeviceProperty = 22,
        GetFloatTrackedDeviceProperty = 23,
        GetInt32TrackedDeviceProperty = 24,
        GetUint64TrackedDeviceProperty = 25,
        GetMatrix34TrackedDeviceProperty = 26,
        GetArrayTrackedDeviceProperty = 27,
        GetStringTrackedDeviceProperty = 28,
        GetPropErrorNameFromEnum = 29,
        PollNextEvent = 30,
        PollNextEventWithPose = 31,
        GetEventTypeNameFromEnum = 32,
        GetHiddenAreaMesh = 33,
        GetControllerState = 34,  // <-- We intercept this!
        GetControllerStateWithPose = 35,  // <-- And this!
        TriggerHapticPulse = 36,
        GetButtonIdNameFromEnum = 37,
        GetControllerAxisTypeNameFromEnum = 38,
        IsInputAvailable = 39,
        IsSteamVRDrawingControllers = 40,
        ShouldApplicationPause = 41,
        ShouldApplicationReduceRenderingWork = 42,
        // ... more functions
    };
}

// ============================================================================
// IVRINPUT WRAPPER
// ============================================================================

// Wrap the IVRInput interface to inject treadmill data
void* WrapIVRInput(void* realInterface);

// IVRInput virtual function indices (from OpenVR SDK)
namespace IVRInputVTable {
    enum {
        SetActionManifestPath = 0,
        GetActionSetHandle = 1,
        GetActionHandle = 2,
        GetInputSourceHandle = 3,
        UpdateActionState = 4,
        GetDigitalActionData = 5,
        GetAnalogActionData = 6,  // <-- We intercept this!
        GetPoseActionData = 7,
        GetSkeletalActionData = 8,
        // ... more functions
    };
}
