// ============================================================================
// TreadmillOpenXRLayer - OpenXR Types and Function Declarations
// ============================================================================
#pragma once

#include "framework.h"

// ============================================================================
// OPENXR TYPES (Minimal definitions)
// ============================================================================

#define XR_DEFINE_HANDLE(object) typedef struct object##_T* object;
#define XR_DEFINE_ATOM(object) typedef uint64_t object;

#define XR_NULL_HANDLE nullptr
#define XR_SUCCEEDED(result) ((result) >= 0)
#define XRAPI_CALL __stdcall
#define XRAPI_PTR *

typedef int64_t XrTime;
typedef int32_t XrResult;
typedef uint64_t XrFlags64;
typedef uint32_t XrBool32;

// Forward declare PFN_xrVoidFunction
typedef void (XRAPI_CALL *PFN_xrVoidFunction)(void);

XR_DEFINE_HANDLE(XrInstance)
XR_DEFINE_HANDLE(XrSession)
XR_DEFINE_HANDLE(XrAction)
XR_DEFINE_HANDLE(XrActionSet)
XR_DEFINE_ATOM(XrPath)

// XrResult values
#define XR_SUCCESS 0
#define XR_ERROR_VALIDATION_FAILURE -1
#define XR_ERROR_RUNTIME_FAILURE -2
#define XR_ERROR_OUT_OF_MEMORY -3
#define XR_ERROR_API_VERSION_UNSUPPORTED -4
#define XR_ERROR_INITIALIZATION_FAILED -5
#define XR_ERROR_FUNCTION_UNSUPPORTED -6
#define XR_ERROR_HANDLE_INVALID -12


// API version
#define XR_CURRENT_API_VERSION XR_MAKE_VERSION(1, 0, 0)
#define XR_MAKE_VERSION(major, minor, patch) \
    ((((uint64_t)(major) & 0xffffULL) << 48) | (((uint64_t)(minor) & 0xffffULL) << 32) | ((uint64_t)(patch) & 0xffffffffULL))

// Loader interface versions
#define XR_CURRENT_LOADER_API_LAYER_VERSION 1
#define XR_LOADER_INFO_STRUCT_VERSION 1
#define XR_API_LAYER_INFO_STRUCT_VERSION 1

// Structure types for loader negotiation
typedef enum XrLoaderInterfaceStructs {
    XR_LOADER_INTERFACE_STRUCT_UNINTIALIZED = 0,
    XR_LOADER_INTERFACE_STRUCT_LOADER_INFO = 1,
    XR_LOADER_INTERFACE_STRUCT_API_LAYER_REQUEST = 2,
    XR_LOADER_INTERFACE_STRUCT_RUNTIME_REQUEST = 3,
    XR_LOADER_INTERFACE_STRUCT_API_LAYER_CREATE_INFO = 4,
    XR_LOADER_INTERFACE_STRUCT_API_LAYER_NEXT_INFO = 5
} XrLoaderInterfaceStructs;

// ============================================================================
// OPENXR STRUCTURES
// ============================================================================

struct XrApiLayerNextInfo;
struct XrApiLayerCreateInfo;
struct XrInstanceCreateInfo;

// Function pointer types - use XRAPI_CALL for calling convention
typedef XrResult (XRAPI_CALL *PFN_xrGetInstanceProcAddr)(XrInstance instance, const char* name, PFN_xrVoidFunction* function);
typedef XrResult (XRAPI_CALL *PFN_xrCreateApiLayerInstance)(const struct XrInstanceCreateInfo* createInfo, const struct XrApiLayerCreateInfo* apiLayerInfo, XrInstance* instance);


struct XrNegotiateLoaderInfo {
    XrLoaderInterfaceStructs structType;
    uint32_t structVersion;
    size_t structSize;
    uint32_t minInterfaceVersion;
    uint32_t maxInterfaceVersion;
    uint64_t minApiVersion;
    uint64_t maxApiVersion;
};

struct XrNegotiateApiLayerRequest {
    XrLoaderInterfaceStructs structType;
    uint32_t structVersion;
    size_t structSize;
    uint32_t layerInterfaceVersion;
    uint64_t layerApiVersion;
    PFN_xrGetInstanceProcAddr getInstanceProcAddr;
    PFN_xrCreateApiLayerInstance createApiLayerInstance;
};

struct XrApiLayerNextInfo {
    XrLoaderInterfaceStructs structType;
    uint32_t structVersion;
    size_t structSize;
    char layerName[256];
    PFN_xrGetInstanceProcAddr nextGetInstanceProcAddr;
    PFN_xrCreateApiLayerInstance nextCreateApiLayerInstance;
    struct XrApiLayerNextInfo* next;
};

struct XrApiLayerCreateInfo {
    XrLoaderInterfaceStructs structType;
    uint32_t structVersion;
    size_t structSize;
    void* loaderInstance;
    char settings_file_location[512];
    XrApiLayerNextInfo* nextInfo;
};

struct XrInstanceCreateInfo {
    uint32_t type;
    const void* next;
    uint32_t createFlags;
    char applicationName[128];
    uint32_t applicationVersion;
    char engineName[128];
    uint32_t engineVersion;
    uint64_t apiVersion;
    uint32_t enabledApiLayerCount;
    const char* const* enabledApiLayerNames;
    uint32_t enabledExtensionCount;
    const char* const* enabledExtensionNames;
};

// Action state structures
struct XrActionStateFloat {
    uint32_t type;
    void* next;
    float currentState;
    XrBool32 changedSinceLastSync;
    XrTime lastChangeTime;
    XrBool32 isActive;
};

struct XrActionStateVector2f {
    uint32_t type;
    void* next;
    float x;
    float y;
    XrBool32 changedSinceLastSync;
    XrTime lastChangeTime;
    XrBool32 isActive;
};

struct XrActionStateGetInfo {
    uint32_t type;
    const void* next;
    XrAction action;
    XrPath subactionPath;
};

struct XrActionsSyncInfo {
    uint32_t type;
    const void* next;
    uint32_t countActiveActionSets;
    const void* activeActionSets;
};

struct XrActionSetCreateInfo {
    uint32_t type;
    const void* next;
    char actionSetName[64];
    char localizedActionSetName[128];
    uint32_t priority;
};

struct XrActionCreateInfo {
    uint32_t type;
    const void* next;
    char actionName[64];
    uint32_t actionType;
    uint32_t countSubactionPaths;
    const XrPath* subactionPaths;
    char localizedActionName[128];
};

// ============================================================================
// LAYER FUNCTION DECLARATIONS
// ============================================================================

// Negotiation entry point
extern "C" XrResult XRAPI_CALL xrNegotiateLoaderApiLayerInterface(
    const XrNegotiateLoaderInfo* loaderInfo,
    const char* layerName,
    XrNegotiateApiLayerRequest* apiLayerRequest
);

// Layer functions
XrResult XRAPI_CALL TreadmillLayer_xrCreateApiLayerInstance(
    const XrInstanceCreateInfo* createInfo,
    const XrApiLayerCreateInfo* apiLayerInfo,
    XrInstance* instance
);

XrResult XRAPI_CALL TreadmillLayer_xrGetInstanceProcAddr(
    XrInstance instance,
    const char* name,
    PFN_xrVoidFunction* function
);

XrResult XRAPI_CALL TreadmillLayer_xrDestroyInstance(XrInstance instance);
XrResult XRAPI_CALL TreadmillLayer_xrGetActionStateFloat(XrSession session, const XrActionStateGetInfo* getInfo, XrActionStateFloat* state);
XrResult XRAPI_CALL TreadmillLayer_xrGetActionStateVector2f(XrSession session, const XrActionStateGetInfo* getInfo, XrActionStateVector2f* state);
XrResult XRAPI_CALL TreadmillLayer_xrSyncActions(XrSession session, const XrActionsSyncInfo* syncInfo);
XrResult XRAPI_CALL TreadmillLayer_xrCreateActionSet(XrInstance instance, const XrActionSetCreateInfo* createInfo, XrActionSet* actionSet);
XrResult XRAPI_CALL TreadmillLayer_xrCreateAction(XrActionSet actionSet, const XrActionCreateInfo* createInfo, XrAction* action);

// Initialize dispatch table
void InitializeDispatchTable(XrInstance instance, PFN_xrGetInstanceProcAddr getInstanceProcAddr);
