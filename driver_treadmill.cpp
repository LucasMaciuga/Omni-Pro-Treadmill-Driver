// Build: x64 DLL
// Dependencies: Windows SDK, openvr_driver.h in project include path.

#include "openvr_driver.h"
#include "TreadmillServerDriver.h"
#include "TreadmillDevice.h"
#include "MinimalOmniReader.h"
#include <atomic>
#include <mutex>
#include <array>
#include <string>
#include <sstream>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <chrono>
#include <debugapi.h>

struct XYState {
    float x = 0.0f;
    float y = 0.0f;
    float yaw = 0.0f;
    
    // Smoothed values
    float x_smoothed = 0.0f;
    float y_smoothed = 0.0f;
    float yaw_smoothed = 0.0f;
    
    uint64_t dataId = 0;  // Timestamp/ID for tracing
    uint64_t logCounter = 0;  // Shared log counter for all components
    
    // Movement tracking for direction analysis
    float lastHmdX = 0.0f;
    float lastHmdZ = 0.0f;
    bool hmdInitialized = false;
    
    std::mutex mtx;
} g_state;

constexpr bool DEBUG_ENABLED = true;

static const char* my_tracker_main_settings_section = "driver_treadmill";
static const char* my_tracker_settings_key_model_number = "mytracker_model_number";
static const char* my_tracker_settings_key_speed_factor = "speed_factor";
static const char* my_tracker_settings_key_smoothing_factor = "smoothing_factor";
static const char* my_tracker_settings_key_com_port = "com_port";
static const char* my_tracker_settings_key_debug = "debug";
static const char* my_tracker_settings_key_omnibridge_dll_path = "omnibridge_dll_path";

std::atomic<bool> g_debug{ DEBUG_ENABLED };
std::atomic<float> g_speedFactor{ 1.0f };
std::atomic<float> g_smoothingFactor{ 0.3f };

void trim(std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    if (start == std::string::npos) { s.clear(); return; }
    s = s.substr(start, end - start + 1);
}

void Log(const char* fmt, ...) {
    if (!g_debug.load()) return;
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    buf[sizeof(buf) - 1] = '\0';

    if (vr::VRDriverLog()) {
        vr::VRDriverLog()->Log(buf);
    }
    else {
        OutputDebugStringA(buf);
        OutputDebugStringA("\n");
    }
}

static void SetDebugFromString(const char* s) {
    if (!s) return;
    std::string ss(s);
    trim(ss);
    std::transform(ss.begin(), ss.end(), ss.begin(), [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
    if (ss == "true" || ss == "1" || ss == "on") g_debug.store(true);
    else g_debug.store(false);
    Log("treadmill: DEBUG set to %d (source=\"%s\")", g_debug.load() ? 1 : 0, s);
}

// TreadmillDevice implementation
TreadmillDevice::TreadmillDevice(unsigned int my_tracker_id) {
    is_active_ = false;
    my_tracker_id_ = my_tracker_id;

    char model_number[1024];
    vr::VRSettings()->GetString(
        my_tracker_main_settings_section, my_tracker_settings_key_model_number, model_number, sizeof(model_number));
    my_device_model_number_ = model_number;

    my_device_serial_number_ = my_device_model_number_ + std::to_string(my_tracker_id);

    input_handles_.fill(vr::k_ulInvalidInputComponentHandle);
    Log("treadmill: My Controller Model Number: %s", my_device_model_number_.c_str());
    Log("treadmill: My Controller Serial Number: %s", my_device_serial_number_.c_str());

    if (vr::VRSettings()) {
        vr::EVRSettingsError se = vr::VRSettingsError_None;
        float v = vr::VRSettings()->GetFloat(my_tracker_main_settings_section, my_tracker_settings_key_speed_factor, &se);
        if (se == vr::VRSettingsError_None && v > 0.0f) {
            g_speedFactor.store(v);
            Log("treadmill: speed_factor loaded from settings: %f", v);
        }
        
        se = vr::VRSettingsError_None;
        float smoothing = vr::VRSettings()->GetFloat(my_tracker_main_settings_section, my_tracker_settings_key_smoothing_factor, &se);
        if (se == vr::VRSettingsError_None && smoothing >= 0.0f && smoothing <= 1.0f) {
            g_smoothingFactor.store(smoothing);
            Log("treadmill: smoothing_factor loaded from settings: %f", smoothing);
        }
    }
}

void TreadmillDevice::UpdateInputs() {
    if (!is_active_) return;
    float x, y, yawDeg;
    uint64_t logCounter;
    { 
        std::lock_guard<std::mutex> lock(g_state.mtx); 
        x = g_state.x_smoothed; 
        y = g_state.y_smoothed; 
        yawDeg = g_state.yaw_smoothed;
        logCounter = g_state.logCounter;
    }

    // CORRECTION: Joystick values are NOT rotated!
    // Rotation happens through the controller's pose rotation
    // X = Sideways (left/right on the treadmill)
    // Y = Forward/Backward (on the treadmill)
    
    float factor = g_speedFactor.load();
    float sx = std::clamp(x * factor, -1.0f, 1.0f);
    float sy = std::clamp(y * factor, -1.0f, 1.0f);

    if (input_handles_[MyComponent_joystick_x] != vr::k_ulInvalidInputComponentHandle) {
        auto e = vr::VRDriverInput()->UpdateScalarComponent(input_handles_[MyComponent_joystick_x], sx, 0.0);
        if (e != vr::VRInputError_None) Log("treadmill: UpdateScalar X failed %d", e);
    }
    if (input_handles_[MyComponent_joystick_y] != vr::k_ulInvalidInputComponentHandle) {
        auto e = vr::VRDriverInput()->UpdateScalarComponent(input_handles_[MyComponent_joystick_y], sy, 0.0);
        if (e != vr::VRInputError_None) Log("treadmill: UpdateScalar Y failed %d", e);
    }
    
    // Unified logging every 50 frames
    if (logCounter % 50 == 0) {
        // ANALYSIS: Is movement correct relative to rotation?
        // If yaw=0° and Y=1.0 (forward) -> should move north
        // If yaw=90° and Y=1.0 (forward) -> should move east
        // If yaw=180° and Y=1.0 (forward) -> should move south
        Log("treadmill: [UpdateInputs #%llu] Controller Yaw=%.1f° | Joystick X=%.3f Y=%.3f | Expected: Y=forward on treadmill, X=sideways",
            logCounter, yawDeg, sx, sy);
    }
}

vr::EVRInitError TreadmillDevice::Activate(vr::TrackedDeviceIndex_t unObjectId) {
    OutputDebugStringA("treadmill: ENTER Activate\n");
    is_active_ = true; 
    m_unObjectId = unObjectId;
    Log("treadmill: Activate called, objectId=%d", static_cast<int>(unObjectId));

    if (!vr::VRProperties()) {
        Log("treadmill: Activate: VRProperties() is null");
        return vr::VRInitError_Driver_Failed;
    }

    auto container = vr::VRProperties()->TrackedDeviceToPropertyContainer(m_unObjectId);
    Log("treadmill: Activate: property container=%llu", static_cast<unsigned long long>(container));

    vr::VRProperties()->SetInt32Property(container, vr::Prop_DeviceClass_Int32, vr::TrackedDeviceClass_Controller);
    vr::VRProperties()->SetStringProperty(container, vr::Prop_ControllerType_String, "treadmill_controller");
    vr::VRProperties()->SetStringProperty(container, vr::Prop_InputProfilePath_String, "{treadmill}/input/treadmill_profile.json");
    vr::VRProperties()->SetStringProperty(container, vr::Prop_SerialNumber_String, "treadmill_xy");
    vr::VRProperties()->SetStringProperty(container, vr::Prop_TrackingSystemName_String, "treadmill");
    vr::VRProperties()->SetStringProperty(container, vr::Prop_ModelNumber_String, my_device_model_number_.c_str());
    vr::VRProperties()->SetStringProperty(container, vr::Prop_RenderModelName_String, "treadmill_controller");
    vr::VRProperties()->SetInt32Property(container, vr::Prop_ControllerRoleHint_Int32, vr::TrackedControllerRole_Treadmill);

    vr::VRProperties()->SetBoolProperty(container, vr::Prop_HasDisplayComponent_Bool, false);
    vr::VRProperties()->SetBoolProperty(container, vr::Prop_HasCameraComponent_Bool, false);
    vr::VRProperties()->SetBoolProperty(container, vr::Prop_HasDriverDirectModeComponent_Bool, false);
    vr::VRProperties()->SetBoolProperty(container, vr::Prop_HasVirtualDisplayComponent_Bool, false);

    vr::EVRInputError err;
    err = vr::VRDriverInput()->CreateScalarComponent(container, "/input/joystick/x", &input_handles_[MyComponent_joystick_x], vr::VRScalarType_Relative, vr::VRScalarUnits_NormalizedTwoSided);
    if (err != vr::VRInputError_None) Log("treadmill: CreateScalar X failed %d", err);

    err = vr::VRDriverInput()->CreateScalarComponent(container, "/input/joystick/y", &input_handles_[MyComponent_joystick_y], vr::VRScalarType_Relative, vr::VRScalarUnits_NormalizedTwoSided);
    if (err != vr::VRInputError_None) Log("treadmill: CreateScalar Y failed %d", err);
    
    m_pose = {};
    m_pose.poseTimeOffset = 0.0;
    m_pose.poseIsValid = true;
    m_pose.deviceIsConnected = true;
    m_pose.result = vr::TrackingResult_Running_OK;
    m_pose.qRotation = { 1.0, 0.0, 0.0, 0.0 };
    m_pose.vecPosition[0] = 0.0;
    m_pose.vecPosition[1] = 0.0;
    m_pose.vecPosition[2] = 1.0;
    m_pose.vecVelocity[0] = m_pose.vecVelocity[1] = m_pose.vecVelocity[2] = 0.0;
    m_pose.vecAcceleration[0] = m_pose.vecAcceleration[1] = m_pose.vecAcceleration[2] = 0.0;
    m_pose.qWorldFromDriverRotation = { 1,0,0,0 };
    m_pose.qDriverFromHeadRotation = { 1,0,0,0 };

    Log("treadmill: Activate: finished for objectId=%d", static_cast<int>(unObjectId));
    return vr::VRInitError_None;
}

void TreadmillDevice::Deactivate() {
    Log("treadmill: Deactivate called for objectId=%d", static_cast<int>(m_unObjectId));
    m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
}

void TreadmillDevice::EnterStandby() {}

void* TreadmillDevice::GetComponent(const char* pchComponentNameAndVersion) { 
    return nullptr; 
}

void TreadmillDevice::DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize) {
    std::string req = pchRequest ? pchRequest : "";
    trim(req);
    Log("treadmill: DebugRequest: \"%s\"", req.c_str());
    if (req.empty()) {
        if (pchResponseBuffer && unResponseBufferSize > 0) {
            strncpy_s(pchResponseBuffer, unResponseBufferSize, "No request", _TRUNCATE);
        }
        return;
    }

    std::istringstream iss(req);
    std::string cmd, arg;
    iss >> cmd >> arg;
    for (auto &c : cmd) c = static_cast<char>(std::tolower((unsigned char)c));

    if (cmd == "debug") {
        SetDebugFromString(arg.c_str());
        if (pchResponseBuffer && unResponseBufferSize > 0) {
            std::string resp = std::string("DEBUG=") + (g_debug.load() ? "true" : "false");
            strncpy_s(pchResponseBuffer, unResponseBufferSize, resp.c_str(), _TRUNCATE);
        }
        return;
    }

    if (cmd == "speed") {
        try {
            float v = std::stof(arg);
            if (v > 0.0f) {
                g_speedFactor.store(v);
                Log("treadmill: speed_factor set via DebugRequest: %f", v);
                if (pchResponseBuffer && unResponseBufferSize > 0) {
                    char resp[64];
                    snprintf(resp, sizeof(resp), "SPEED=%g", static_cast<double>(v));
                    strncpy_s(pchResponseBuffer, unResponseBufferSize, resp, _TRUNCATE);
                }
                return;
            }
        } catch (...) {}
        if (pchResponseBuffer && unResponseBufferSize > 0) {
            strncpy_s(pchResponseBuffer, unResponseBufferSize, "Invalid SPEED", _TRUNCATE);
        }
        return;
    }

    if (cmd == "smoothing") {
        try {
            float v = std::stof(arg);
            if (v >= 0.0f && v <= 1.0f) {
                g_smoothingFactor.store(v);
                Log("treadmill: smoothing_factor set via DebugRequest: %f", v);
                if (pchResponseBuffer && unResponseBufferSize > 0) {
                    char resp[64];
                    snprintf(resp, sizeof(resp), "SMOOTHING=%g", static_cast<double>(v));
                    strncpy_s(pchResponseBuffer, unResponseBufferSize, resp, _TRUNCATE);
                }
                return;
            }
        } catch (...) {}
        if (pchResponseBuffer && unResponseBufferSize > 0) {
            strncpy_s(pchResponseBuffer, unResponseBufferSize, "Invalid SMOOTHING (0.0-1.0)", _TRUNCATE);
        }
        return;
    }

    if (pchResponseBuffer && unResponseBufferSize > 0) {
        strncpy_s(pchResponseBuffer, unResponseBufferSize, "Unknown command", _TRUNCATE);
    }
}

vr::DriverPose_t TreadmillDevice::GetPose() {
    float rawYaw;
    uint64_t dataId;
    
    {
        std::lock_guard<std::mutex> lock(g_state.mtx);
        rawYaw = g_state.yaw_smoothed;
        dataId = g_state.dataId;
        
        m_pose.poseIsValid = true;
        m_pose.deviceIsConnected = true;
        m_pose.result = vr::TrackingResult_Running_OK;

        // Position remains at (0,0,0) - the controller does not move in space
        m_pose.vecPosition[0] = 0.0;
        m_pose.vecPosition[1] = 0.0;
        m_pose.vecPosition[2] = 0.0;

        // IMPORTANT: The controller's rotation indicates which direction the treadmill is pointing
        // The game then interprets: "Joystick forward" + "Controller points east" = "Move east"
        constexpr double DEG2RAD = 3.14159265358979323846 / 180.0;
        double theta = static_cast<double>(rawYaw) * DEG2RAD;
        
        double half = theta * 0.5;
        double c = std::cos(half);
        double s = std::sin(half);
        
        // Rotation around Y-axis - NEGATIVE sign because treadmill rotates in opposite direction
        m_pose.qRotation.w = c;
        m_pose.qRotation.x = 0.0;
        m_pose.qRotation.y = -s;  // CHANGED: from s to -s
        m_pose.qRotation.z = 0.0;
    }
    
    // Debug logging AFTER lock
    static int frameCount = 0;
    if (++frameCount % 100 == 0) {
        Log("treadmill: [TreadmillDevice::GetPose ID=%llu] SMOOTHED yaw=%.2f° | CALC quat(w=%.4f, x=%.4f, y=%.4f, z=%.4f)",
            dataId, rawYaw, 
            m_pose.qRotation.w, m_pose.qRotation.x, m_pose.qRotation.y, m_pose.qRotation.z);
    }

    return m_pose;
}

void OnOmniData(float ringAngle, int gamePadX, int gamePadY)
{
    // Generate timestamp for tracing
    uint64_t timestamp = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
    
    {
        std::lock_guard<std::mutex> lock(g_state.mtx);
        
        // X remains the same (left/right)
        float raw_x = std::clamp((static_cast<float>(gamePadX) - 127.0f) / 127.0f, -1.0f, 1.0f);
        
        // Y inverted (forward/backward)
        float raw_y = std::clamp(-(static_cast<float>(gamePadY) - 127.0f) / 127.0f, -1.0f, 1.0f);
        
        // Store raw values
        g_state.x = raw_x;
        g_state.y = raw_y;
        g_state.yaw = ringAngle;
        
        // Apply exponential moving average (EMA) smoothing
        float alpha = g_smoothingFactor.load();
        
        // For movement (X, Y) - simple EMA
        g_state.x_smoothed = alpha * raw_x + (1.0f - alpha) * g_state.x_smoothed;
        g_state.y_smoothed = alpha * raw_y + (1.0f - alpha) * g_state.y_smoothed;
        
        // For rotation (Yaw) - handle angle wrapping (0-360 degrees)
        float yaw_diff = ringAngle - g_state.yaw_smoothed;
        
        // Normalize angle difference to [-180, 180]
        if (yaw_diff > 180.0f) yaw_diff -= 360.0f;
        if (yaw_diff < -180.0f) yaw_diff += 360.0f;
        
        // Apply smoothing to the difference
        g_state.yaw_smoothed += alpha * yaw_diff;
        
        // Normalize smoothed yaw to [0, 360]
        if (g_state.yaw_smoothed < 0.0f) g_state.yaw_smoothed += 360.0f;
        if (g_state.yaw_smoothed >= 360.0f) g_state.yaw_smoothed -= 360.0f;
        
        g_state.dataId = timestamp;
        g_state.logCounter++;
    }
    
    // Unified logging every 50 frames
    if (g_state.logCounter % 50 == 0) {
        Log("treadmill: [OnOmniData #%llu] RAW: angle=%.2f° X=%d Y=%d | SMOOTHED: angle=%.2f° X=%.3f Y=%.3f",
            g_state.logCounter, ringAngle, gamePadX, gamePadY,
            g_state.yaw_smoothed, g_state.x_smoothed, g_state.y_smoothed);
    }
}


// NEW: Implementation of visualization tracker
vr::EVRInitError TreadmillVisualTracker::Activate(vr::TrackedDeviceIndex_t unObjectId) {
    m_unObjectId = unObjectId;
    Log("treadmill: VisualTracker Activate called, objectId=%d", static_cast<int>(unObjectId));

    if (!vr::VRProperties()) {
        Log("treadmill: VisualTracker: VRProperties() is null");
        return vr::VRInitError_Driver_Failed;
    }

    auto container = vr::VRProperties()->TrackedDeviceToPropertyContainer(m_unObjectId);

    // Register as GenericTracker (visible!)
    vr::VRProperties()->SetInt32Property(container, vr::Prop_DeviceClass_Int32, vr::TrackedDeviceClass_GenericTracker);
    
    // Base properties
    vr::VRProperties()->SetStringProperty(container, vr::Prop_TrackingSystemName_String, "treadmill");
    vr::VRProperties()->SetStringProperty(container, vr::Prop_ModelNumber_String, "Treadmill_Orientation_Tracker");
    vr::VRProperties()->SetStringProperty(container, vr::Prop_SerialNumber_String, "treadmill_visual_001");
    vr::VRProperties()->SetStringProperty(container, vr::Prop_RenderModelName_String, "{htc}vr_tracker_vive_1_0");
    vr::VRProperties()->SetStringProperty(container, vr::Prop_ManufacturerName_String, "Treadmill");
    
    // Icons for SteamVR (uses Vive Tracker icons)
    vr::VRProperties()->SetStringProperty(container, vr::Prop_NamedIconPathDeviceOff_String, "{htc}/icons/tracker_status_off.png");
    vr::VRProperties()->SetStringProperty(container, vr::Prop_NamedIconPathDeviceSearching_String, "{htc}/icons/tracker_status_searching.gif");
    vr::VRProperties()->SetStringProperty(container, vr::Prop_NamedIconPathDeviceSearchingAlert_String, "{htc}/icons/tracker_status_searching_alert.gif");
    vr::VRProperties()->SetStringProperty(container, vr::Prop_NamedIconPathDeviceReady_String, "{htc}/icons/tracker_status_ready.png");
    vr::VRProperties()->SetStringProperty(container, vr::Prop_NamedIconPathDeviceReadyAlert_String, "{htc}/icons/tracker_status_ready_alert.png");
    vr::VRProperties()->SetStringProperty(container, vr::Prop_NamedIconPathDeviceNotReady_String, "{htc}/icons/tracker_status_error.png");
    vr::VRProperties()->SetStringProperty(container, vr::Prop_NamedIconPathDeviceStandby_String, "{htc}/icons/tracker_status_standby.png");
    vr::VRProperties()->SetStringProperty(container, vr::Prop_NamedIconPathDeviceAlertLow_String, "{htc}/icons/tracker_status_ready_low.png");

    // Tracker-specific properties
    vr::VRProperties()->SetBoolProperty(container, vr::Prop_WillDriftInYaw_Bool, false);
    vr::VRProperties()->SetBoolProperty(container, vr::Prop_DeviceIsWireless_Bool, false);
    vr::VRProperties()->SetBoolProperty(container, vr::Prop_DeviceIsCharging_Bool, false);
    vr::VRProperties()->SetFloatProperty(container, vr::Prop_DeviceBatteryPercentage_Float, 1.0f);
    
    // Tracking properties
    vr::VRProperties()->SetBoolProperty(container, vr::Prop_Identifiable_Bool, true);
    vr::VRProperties()->SetInt32Property(container, vr::Prop_Axis0Type_Int32, vr::k_eControllerAxis_None);
    vr::VRProperties()->SetInt32Property(container, vr::Prop_Axis1Type_Int32, vr::k_eControllerAxis_None);
    vr::VRProperties()->SetInt32Property(container, vr::Prop_Axis2Type_Int32, vr::k_eControllerAxis_None);
    vr::VRProperties()->SetInt32Property(container, vr::Prop_Axis3Type_Int32, vr::k_eControllerAxis_None);
    vr::VRProperties()->SetInt32Property(container, vr::Prop_Axis4Type_Int32, vr::k_eControllerAxis_None);
    
    // Explicitly disable controller role
    vr::VRProperties()->SetInt32Property(container, vr::Prop_ControllerRoleHint_Int32, vr::TrackedControllerRole_Invalid);

    // Initialize pose
    m_pose = {};
    m_pose.poseTimeOffset = 0.0;
    m_pose.poseIsValid = true;
    m_pose.deviceIsConnected = true;
    m_pose.result = vr::TrackingResult_Running_OK;
    m_pose.qRotation = { 1.0, 0.0, 0.0, 0.0 };
    
    // Position: in front of player at chest height
    m_pose.vecPosition[0] = 0.0;
    m_pose.vecPosition[1] = 1.2;
    m_pose.vecPosition[2] = -0.5;
    

    Log("treadmill: VisualTracker activated successfully");
    return vr::VRInitError_None;
}

void TreadmillVisualTracker::Deactivate() {
    Log("treadmill: VisualTracker Deactivate called");
    m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
}

void TreadmillVisualTracker::EnterStandby() {}

void* TreadmillVisualTracker::GetComponent(const char* pchComponentNameAndVersion) {
    return nullptr;
}

void TreadmillVisualTracker::DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize) {
    if (pchResponseBuffer && unResponseBufferSize > 0) {
        strncpy_s(pchResponseBuffer, unResponseBufferSize, "VisualTracker", _TRUNCATE);
    }
}

vr::DriverPose_t TreadmillVisualTracker::GetPose() {
    float rawYaw;
    uint64_t dataId;
    uint64_t logCounter;
    float joystickX, joystickY;
    
    {
        std::lock_guard<std::mutex> lock(g_state.mtx);
        rawYaw = g_state.yaw_smoothed;
        dataId = g_state.dataId;
        logCounter = g_state.logCounter;
        joystickX = g_state.x_smoothed;
        joystickY = g_state.y_smoothed;
        
        m_pose.poseIsValid = true;
        m_pose.deviceIsConnected = true;
        m_pose.result = vr::TrackingResult_Running_OK;

        // Tracker positions itself relative to HMD
        vr::TrackedDevicePose_t hmdPose;
        vr::VRServerDriverHost()->GetRawTrackedDevicePoses(0.0f, &hmdPose, 1);
        
        float currentHmdX = 0.0f;
        float currentHmdZ = 0.0f;
        bool hmdValid = false;
        
        if (hmdPose.bPoseIsValid) {
            vr::HmdMatrix34_t hmdMatrix = hmdPose.mDeviceToAbsoluteTracking;
            
            currentHmdX = hmdMatrix.m[0][3];
            currentHmdZ = hmdMatrix.m[2][3];
            hmdValid = true;
            
            // Position: Follows HMD position, but NOT HMD rotation
            m_pose.vecPosition[0] = currentHmdX;                  // X position from HMD
            m_pose.vecPosition[1] = hmdMatrix.m[1][3] - 0.3;     // Y position from HMD, 0.3m lower (chest height)
            m_pose.vecPosition[2] = currentHmdZ - 0.5;            // Z position from HMD, 0.5m forward
        } else {
            // Fallback if HMD pose is invalid
            m_pose.vecPosition[0] = 0.0;
            m_pose.vecPosition[1] = 1.2;
            m_pose.vecPosition[2] = -0.5;
        }

        // MOVEMENT ANALYSIS: Actual vs Expected direction
        if (hmdValid && g_state.hmdInitialized && (logCounter % 50 == 0)) {
            // Calculate actual movement (in world coordinates)
            float actualDeltaX = currentHmdX - g_state.lastHmdX;
            float actualDeltaZ = currentHmdZ - g_state.lastHmdZ;
            float actualDistance = std::sqrt(actualDeltaX * actualDeltaX + actualDeltaZ * actualDeltaZ);
            
            // Only analyze if significant movement exists (>5cm)
            if (actualDistance > 0.05f) {
                // Normalized actual direction
                float actualDirX = actualDeltaX / actualDistance;
                float actualDirZ = actualDeltaZ / actualDistance;
                
                // Calculate expected direction based on treadmill rotation and joystick
                constexpr double DEG2RAD = 3.14159265358979323846 / 180.0;
                double yawRad = static_cast<double>(rawYaw) * DEG2RAD;
                
                // Rotate joystick values into world coordinates
                // Joystick: X=sideways, Y=forward on treadmill
                // World: X=right, Z=forward (negative)
                double sinYaw = std::sin(yawRad);
                double cosYaw = std::cos(yawRad);
                
                float expectedWorldX = static_cast<float>(joystickX * cosYaw - joystickY * sinYaw);
                float expectedWorldZ = static_cast<float>(joystickX * sinYaw + joystickY * cosYaw);
                
                float expectedLength = std::sqrt(expectedWorldX * expectedWorldX + expectedWorldZ * expectedWorldZ);
                if (expectedLength > 0.01f) {
                    expectedWorldX /= expectedLength;
                    expectedWorldZ /= expectedLength;
                    
                    // Calculate angle deviation between actual and expected direction
                    float dotProduct = actualDirX * expectedWorldX + actualDirZ * expectedWorldZ;
                    dotProduct = std::clamp(dotProduct, -1.0f, 1.0f);
                    float angleDiff = std::acos(dotProduct) * 180.0f / 3.14159265358979323846f;
                    
                    // WARNING on large deviation (>5°)
                    if (angleDiff > 5.0f) {
                        Log("treadmill: [DIRECTION MISMATCH!] Angle Deviation: %.1f° | Actual: X=%.3f Z=%.3f | Expected: X=%.3f Z=%.3f | Treadmill Yaw=%.1f° | Joystick X=%.2f Y=%.2f",
                            angleDiff,
                            actualDirX, actualDirZ,
                            expectedWorldX, expectedWorldZ,
                            rawYaw, joystickX, joystickY);
                    } else {
                        Log("treadmill: [Direction OK] Deviation: %.1f° | Actual: X=%.3f Z=%.3f | Expected: X=%.3f Z=%.3f",
                            angleDiff, actualDirX, actualDirZ, expectedWorldX, expectedWorldZ);
                    }
                }
            }
        }
        
        // Store current position for next frame
        if (hmdValid) {
            g_state.lastHmdX = currentHmdX;
            g_state.lastHmdZ = currentHmdZ;
            g_state.hmdInitialized = true;
        }

        // Tracker rotation based ONLY on treadmill yaw (NOT HMD rotation!)
        constexpr double DEG2RAD = 3.14159265358979323846 / 180.0;
        double theta = static_cast<double>(rawYaw) * DEG2RAD;
        
        // Calculate quaternion
        double half = theta * 0.5;
        double c = std::cos(half);
        double s = std::sin(half);
        
        m_pose.qRotation.w = c;
        m_pose.qRotation.x = 0.0;
        m_pose.qRotation.y = -s;
        m_pose.qRotation.z = 0.0;
        
        // Normalize quaternion
        double qLength = std::sqrt(
            m_pose.qRotation.w * m_pose.qRotation.w +
            m_pose.qRotation.x * m_pose.qRotation.x +
            m_pose.qRotation.y * m_pose.qRotation.y +
            m_pose.qRotation.z * m_pose.qRotation.z
        );
        
        if (qLength > 0.0001) {
            m_pose.qRotation.w /= qLength;
            m_pose.qRotation.x /= qLength;
            m_pose.qRotation.y /= qLength;
            m_pose.qRotation.z /= qLength;
        }
    }

    // Unified logging every 50 frames
    if (logCounter % 50 == 0) {
        double expectedWorldX = std::sin(rawYaw * 3.14159265358979323846 / 180.0);
        double expectedWorldZ = -std::cos(rawYaw * 3.14159265358979323846 / 180.0);
        
        Log("treadmill: [VisualTracker::GetPose #%llu] Treadmill Yaw=%.2f° | Quat(w=%.4f, y=%.4f) | Expected Direction: X=%.3f Z=%.3f | Pos(%.2f, %.2f, %.2f)",
            logCounter, rawYaw,
            m_pose.qRotation.w, m_pose.qRotation.y,
            expectedWorldX, expectedWorldZ,
            m_pose.vecPosition[0], m_pose.vecPosition[1], m_pose.vecPosition[2]);
    }

    return m_pose;
}

extern "C" __declspec(dllexport) void* HmdDriverFactory(const char* pInterfaceName, int* pReturnCode) {
    try {
        if (pReturnCode) *pReturnCode = vr::VRInitError_Init_InterfaceNotFound;
        char buf[256];
        snprintf(buf, sizeof(buf), "HmdDriverFactory called for interface '%s'", pInterfaceName ? pInterfaceName : "<null>");
        OutputDebugStringA(buf);
        if (pInterfaceName && 0 == strcmp(vr::IServerTrackedDeviceProvider_Version, pInterfaceName)) { 
            static TreadmillServerDriver serverDriver;
            if (pReturnCode) *pReturnCode = vr::VRInitError_None; 
            OutputDebugStringA("HmdDriverFactory: returning TreadmillServerDriver");
            return &serverDriver;
        }
        OutputDebugStringA("HmdDriverFactory: interface not found");
        return nullptr;
    }
    catch (const std::exception& e) {
        if (pReturnCode) *pReturnCode = vr::VRInitError_Driver_Failed;
        char buf[256];
        snprintf(buf, sizeof(buf), "HmdDriverFactory threw std::exception: %s", e.what());
        OutputDebugStringA(buf);
        return nullptr;
    }
    catch (...) {
        if (pReturnCode) *pReturnCode = vr::VRInitError_Driver_Failed;
        OutputDebugStringA("HmdDriverFactory threw unknown exception");
        return nullptr;
    }
}