#include "pch.h"
// ============================================================================
// TreadmillOpenXRLayer - OpenXR API Layer Entry Point
// ============================================================================
// This is an OpenXR API Layer that intercepts input-related calls to inject
// treadmill movement data into any OpenXR application.
//
// Layer registration (Windows):
// - Add to registry: HKLM\SOFTWARE\Khronos\OpenXR\1\ApiLayers\Implicit
// - Key: Path to manifest JSON file
// - Value: 0 (DWORD, enabled)
// ============================================================================

using namespace TreadmillLayer;

// ============================================================================
// GLOBALS
// ============================================================================

static HMODULE g_thisModule = nullptr;
static bool g_initialized = false;
static XrInstance g_instance = XR_NULL_HANDLE;

// Dispatch table for next layer/runtime
static PFN_xrGetInstanceProcAddr g_nextGetInstanceProcAddr = nullptr;

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

static void InitializeLayer() {
    if (g_initialized) return;
    
    std::wstring moduleDir = GetModuleDirectory(g_thisModule);
    
    // Initialize logging
    g_config.logPath = moduleDir + L"\\treadmill_layer.log";
    InitLogging(g_config.logPath);
    
    Log("========================================");
    Log("TreadmillOpenXRLayer Initializing");
    Log("========================================");
    
    // Load configuration
    std::wstring configPath = moduleDir + L"\\treadmill_layer_config.json";
    g_config = Config::Load(configPath);
    
    Log("Configuration:");
    Log("  Enabled: %s", g_config.enabled ? "true" : "false");
    Log("  COM Port: %s", g_config.comPort.c_str());
    Log("  Speed Multiplier: %.2f", g_config.speedMultiplier);
    
    // Initialize treadmill connection
    if (g_config.enabled) {
        std::wstring omniBridgePath = moduleDir + L"\\OmniBridge.dll";
        if (OmniBridge::Initialize(omniBridgePath, g_config.comPort, g_config.baudRate)) {
            Log("Treadmill input active!");
        } else {
            Log("WARNING: Treadmill not connected");
        }
    }
    
    g_initialized = true;
    Log("Layer initialization complete!");
}

static void ShutdownLayer() {
    Log("Shutting down layer...");
    OmniBridge::Shutdown();
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
        break;
        
    case DLL_PROCESS_DETACH:
        if (reserved == nullptr) {
            ShutdownLayer();
        }
        break;
    }
    return TRUE;
}

// ============================================================================
// OPENXR LAYER NEGOTIATION
// ============================================================================

extern "C" {

// This is the entry point that the OpenXR loader calls to negotiate with the layer
XrResult XRAPI_CALL xrNegotiateLoaderApiLayerInterface(
    const XrNegotiateLoaderInfo* loaderInfo,
    const char* layerName,
    XrNegotiateApiLayerRequest* apiLayerRequest
) {
    InitializeLayer();
    
    Log("xrNegotiateLoaderApiLayerInterface called");
    Log("  Layer name: %s", layerName);
    
    if (!loaderInfo || !apiLayerRequest) {
        Log("ERROR: Invalid parameters");
        return XR_ERROR_INITIALIZATION_FAILED;
    }
    
    if (loaderInfo->structType != XR_LOADER_INTERFACE_STRUCT_LOADER_INFO ||
        loaderInfo->structVersion != XR_LOADER_INFO_STRUCT_VERSION ||
        loaderInfo->structSize != sizeof(XrNegotiateLoaderInfo)) {
        Log("ERROR: Invalid loader info structure");
        return XR_ERROR_INITIALIZATION_FAILED;
    }
    
    if (apiLayerRequest->structType != XR_LOADER_INTERFACE_STRUCT_API_LAYER_REQUEST ||
        apiLayerRequest->structVersion != XR_API_LAYER_INFO_STRUCT_VERSION ||
        apiLayerRequest->structSize != sizeof(XrNegotiateApiLayerRequest)) {
        Log("ERROR: Invalid API layer request structure");
        return XR_ERROR_INITIALIZATION_FAILED;
    }
    
    // Check API version compatibility
    if (loaderInfo->minInterfaceVersion > XR_CURRENT_LOADER_API_LAYER_VERSION ||
        loaderInfo->maxInterfaceVersion < XR_CURRENT_LOADER_API_LAYER_VERSION) {
        Log("ERROR: Incompatible interface version");
        return XR_ERROR_INITIALIZATION_FAILED;
    }
    
    // Fill in the layer's response
    apiLayerRequest->layerInterfaceVersion = XR_CURRENT_LOADER_API_LAYER_VERSION;
    apiLayerRequest->layerApiVersion = XR_CURRENT_API_VERSION;
    apiLayerRequest->getInstanceProcAddr = TreadmillLayer_xrGetInstanceProcAddr;
    apiLayerRequest->createApiLayerInstance = TreadmillLayer_xrCreateApiLayerInstance;
    
    Log("Layer negotiation successful");
    
    return XR_SUCCESS;
}

} // extern "C"

// ============================================================================
// LAYER INSTANCE CREATION
// ============================================================================

XrResult XRAPI_CALL TreadmillLayer_xrCreateApiLayerInstance(
    const XrInstanceCreateInfo* createInfo,
    const XrApiLayerCreateInfo* apiLayerInfo,
    XrInstance* instance
) {
    Log("TreadmillLayer_xrCreateApiLayerInstance called");
    
    if (!createInfo || !apiLayerInfo || !instance) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    // Store the next layer's getInstanceProcAddr
    g_nextGetInstanceProcAddr = apiLayerInfo->nextInfo->nextGetInstanceProcAddr;
    
    // Create the instance using the next layer/runtime
    PFN_xrCreateApiLayerInstance nextCreateInstance = nullptr;
    
    // Get the next create instance function
    XrApiLayerCreateInfo nextLayerInfo = *apiLayerInfo;
    nextLayerInfo.nextInfo = apiLayerInfo->nextInfo->next;
    
    // Call the next layer's create function
    auto createFunc = apiLayerInfo->nextInfo->nextCreateApiLayerInstance;
    XrResult result = createFunc(createInfo, &nextLayerInfo, instance);
    
    if (XR_SUCCEEDED(result)) {
        g_instance = *instance;
        Log("Instance created successfully: 0x%llX", (uint64_t)*instance);
        
        // Initialize our dispatch table
        InitializeDispatchTable(*instance, g_nextGetInstanceProcAddr);
    }
    
    return result;
}

// ============================================================================
// FUNCTION INTERCEPTION
// ============================================================================

XrResult XRAPI_CALL TreadmillLayer_xrGetInstanceProcAddr(
    XrInstance instance,
    const char* name,
    PFN_xrVoidFunction* function
) {
    if (!name || !function) {
        return XR_ERROR_VALIDATION_FAILURE;
    }
    
    // Check if we're intercepting this function
    if (strcmp(name, "xrGetActionStateFloat") == 0) {
        *function = (PFN_xrVoidFunction)TreadmillLayer_xrGetActionStateFloat;
        return XR_SUCCESS;
    }
    
    if (strcmp(name, "xrGetActionStateVector2f") == 0) {
        *function = (PFN_xrVoidFunction)TreadmillLayer_xrGetActionStateVector2f;
        return XR_SUCCESS;
    }
    
    if (strcmp(name, "xrSyncActions") == 0) {
        *function = (PFN_xrVoidFunction)TreadmillLayer_xrSyncActions;
        return XR_SUCCESS;
    }
    
    if (strcmp(name, "xrCreateActionSet") == 0) {
        *function = (PFN_xrVoidFunction)TreadmillLayer_xrCreateActionSet;
        return XR_SUCCESS;
    }
    
    if (strcmp(name, "xrCreateAction") == 0) {
        *function = (PFN_xrVoidFunction)TreadmillLayer_xrCreateAction;
        return XR_SUCCESS;
    }
    
    if (strcmp(name, "xrDestroyInstance") == 0) {
        *function = (PFN_xrVoidFunction)TreadmillLayer_xrDestroyInstance;
        return XR_SUCCESS;
    }
    
    // Pass through to next layer
    if (g_nextGetInstanceProcAddr) {
        return g_nextGetInstanceProcAddr(instance, name, function);
    }
    
    return XR_ERROR_FUNCTION_UNSUPPORTED;
}
