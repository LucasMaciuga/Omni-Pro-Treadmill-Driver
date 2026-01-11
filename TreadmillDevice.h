#pragma once

#include "openvr_driver.h"
#include <atomic>
#include <array>
#include <string>
#include <mutex>

#define _AMD64_

extern std::atomic<float> g_speedFactor;

void Log(const char* fmt, ...);

enum MyComponent
{
    MyComponent_joystick_x,
    MyComponent_joystick_y,
    MyComponent_MAX
};

// Original Treadmill-Controller (invisible, input-only)
class TreadmillDevice : public vr::ITrackedDeviceServerDriver {
private:
    std::atomic<bool> is_active_;
    std::string my_device_model_number_;
    std::string my_device_serial_number_;
    unsigned int my_tracker_id_;
    vr::DriverPose_t m_pose{};
    std::array<vr::VRInputComponentHandle_t, MyComponent_MAX> input_handles_;

public:
    vr::TrackedDeviceIndex_t m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;
    
    TreadmillDevice(unsigned int my_tracker_id);
    
    void UpdateInputs();
    vr::DriverPose_t GetPose();
    
    // ITrackedDeviceServerDriver overrides
    vr::EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId) override;
    void Deactivate() override;
    void EnterStandby() override;
    void* GetComponent(const char* pchComponentNameAndVersion) override;
    void DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize) override;
};

// NEW: Visualization tracker (visible in SteamVR)
class TreadmillVisualTracker : public vr::ITrackedDeviceServerDriver {
public:
    vr::TrackedDeviceIndex_t m_unObjectId = vr::k_unTrackedDeviceIndexInvalid;  // <-- PUBLIC!
    vr::DriverPose_t m_pose{};

    TreadmillVisualTracker() = default;
    
    vr::DriverPose_t GetPose();
    
    vr::EVRInitError Activate(vr::TrackedDeviceIndex_t unObjectId) override;
    void Deactivate() override;
    void EnterStandby() override;
    void* GetComponent(const char* pchComponentNameAndVersion) override;
    void DebugRequest(const char* pchRequest, char* pchResponseBuffer, uint32_t unResponseBufferSize) override;
};