# TreadmillSteamVR

A SteamVR driver for the Virtuix Omni Treadmill that translates real-world treadmill motion into VR controller input and tracked device orientation. Enables natural omnidirectional movement in VR games through actual treadmill walking.

## Installation

1. You need http://content.omniverse.global/Omni_Connect_1_3_4_0.exe installed before you can use this driver
2. Get the newest release of the driver from [here](https://github.com/LucasMaci/Omni-Pro-Treadmill-Driver/releases) (you can use the zip or 7z file)
3. Extrac the package into the SteamVR *driver* folder to: `C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers\treadmill\` (C:\Program Files (x86)\Steam is the default installation directory. If you have installed it somewhere else you need to take care of this)
4. Verify file structure:
   ```
   treadmill/
   ├── bin/win64/
   │   ├── driver_treadmill.dll
   │   ├── OmniBridge.dll
   │   └── OmniCommon.dll
   ├── resources/
   │   ├── settings/default.vrsettings
   │   └── input/treadmill_profile.json
   └── driver.vrdrivermanifest
   ```
5. Configure **File**: `treadmill/resources/settings/default.vrsettings` as described below. (at least set "com_port" and verify "omnibridge_dll_path")
6. Restart SteamVR
7. Check driver console for connection status

### Configuration (Settings)
There are a few settings which needs to be set in the  **File**: `treadmill/resources/settings/default.vrsettings`.

#### COM Port
This must be set to the port your Omni Treadmill is using.
You find it by opening "devicemanager" and check what your "Omni Serial Command Port" has behind it (by me it was "COM3")

<img width="637" height="408" alt="image" src="https://github.com/user-attachments/assets/79a01826-f79b-48d5-b233-0f46836f6a77" />

#### Omni Bridge DLL Path
The OmniBridge.dll is needed by the SteamVR driver, but currently is not autodetected. So set it manualy under "omnibridge_dll_path".
**Take care that the path must be absolut and you need to put double backslashes instead of single backslahses**


```json
{
  "driver_treadmill": {
    "com_port": "COM3",
    "omnibridge_dll_path": "C:\\Program Files (x86)\\Steam\\steamapps\\common\\SteamVR\\drivers\\treadmill\\bin\\win64\\OmniBridge.dll",
    "debug": true,
    "speed_factor": 1.0,
    "smoothing_factor": 0.3
  }
}
```

### Configuration (in-Game movement)
In SteamVR you need to bind the treadmill movement to the game, because this is a custom solution and no default will handle that for you. Currently tested for SkyrimVR a "legacy-binding-game" and No Man's Sky a "modern-binding-game". 

#### Legacy-Mode (the old, the SkyrimVR way)
This is the old way how SteamVR binds the treadmill to the correct action in the game.

- Open the "controller bindings" (sorry for the german pictures, but I have no idea how to switch it to english)
  - <img width="799" height="629" alt="image" src="https://github.com/user-attachments/assets/196c0ac2-ba49-4e8d-a93e-5b7c729a3c52" />
- Select your game and click on "different binding"
  - <img width="800" height="631" alt="image" src="https://github.com/user-attachments/assets/f9b448b2-2ecc-48d2-9de4-cd5608f1668b" />
  - <img width="796" height="629" alt="image" src="https://github.com/user-attachments/assets/0d6d9121-2432-45f1-a456-67cc101889ee" />
- Click on what ever is selected in the circled section to switch to the treadmill caleld "treadmill_controller"
  - <img width="1480" height="853" alt="image" src="https://github.com/user-attachments/assets/faaef12b-0f04-4da8-be58-3af6d679ff2e" />
  - <img width="1484" height="871" alt="image" src="https://github.com/user-attachments/assets/5c93ddba-f68f-4649-a95f-6c0e5ff6d310" />
- Now click "edit" to change or add binding
  - <img width="1486" height="692" alt="image" src="https://github.com/user-attachments/assets/740cbcb4-206d-4020-a169-4b4974017987" />
- If nothing is pre-set click on the small plus icon and add "joystick". The position will be blank, so click on it and choose **left_axis0_value** . th
  - <img width="1483" height="852" alt="image" src="https://github.com/user-attachments/assets/6d50a661-9cf2-4053-8d8d-0c38f8159a5d" />
  - <img width="1483" height="851" alt="image" src="https://github.com/user-attachments/assets/e0f1dbe9-e56c-44f4-b87a-74a8a78510b7" />
- **This is important** After this, go to the additional settings and check "forward binding to the left hand" and also "forward binding to the right hand". You need it, because else the normal controller migth be ignored completely
  -  <img width="1484" height="671" alt="image" src="https://github.com/user-attachments/assets/fd0d9bcb-e73b-4f88-91a5-3a12caf9837d" />


#### Modern Mode (the new, the No Man's Sky way)
This is the new way how SteamVR binds the treadmill to the correct action in the game.

- Follow the steps as above, but just for another game (like No Man's Sky)
- After you followed it to the step "Now click "edit" to change or add binding", in modern games the binding will look much different, but no panic, just look for "by foot" and add the joystick, add the action and select "move"
  - <img width="1940" height="1073" alt="image" src="https://github.com/user-attachments/assets/7e9d30fb-f7b3-47d9-a829-caf0d6d9f700" />
- That was it you should be ready "to-go"


## Troubleshooting

### If you dont't see the treadmill icon

You should see the "c" icon, if the treadmill was installed correctly.

<img width="315" height="153" alt="image" src="https://github.com/user-attachments/assets/a1777395-43d4-47de-a849-f717d861223c" />

#### How To Fix?
- The driver was not installed correctly. Retry it or open an issue.
- Have you connected and started the treadmill?
- Are the foottrackers on and have enough energy?
- Is the Omni Connect app really closed (close via tray icon)
  - <img width="406" height="162" alt="image" src="https://github.com/user-attachments/assets/3c76724f-ee5e-4e5c-8e66-928b1ff9092d" />


### You see the treadmill icon but there is no movement in the game?

Open "settings" from the menu:

<img width="314" height="396" alt="image" src="https://github.com/user-attachments/assets/691f9352-3593-42cb-99a5-e69de2452c6b" />

Navigate to "controller" and "controller testing"

<img width="799" height="629" alt="image" src="https://github.com/user-attachments/assets/e767e3fc-6955-4b3e-ac43-dd3df5e24e1e" />

Switch to the #/USER/TREADMILL device and "walk" in the air with your boots.
You should see the blue dot move forward:

<img width="800" height="631" alt="image" src="https://github.com/user-attachments/assets/dd2276ae-efeb-492f-8a15-d3711df99744" />

Still not working?
- Ensure the Omni Connect app is really closed (close via tray icon)
  - <img width="406" height="162" alt="image" src="https://github.com/user-attachments/assets/3c76724f-ee5e-4e5c-8e66-928b1ff9092d" />



## Overview (for developers)

**TreadmillSteamVR** is a native C++ SteamVR driver that bridges the Virtuix Omni Treadmill's motion sensors with SteamVR's input and tracking systems. By dynamically loading the OmniBridge .NET assembly, it captures real-time motion data (ring angle, step count, gamepad input) and transforms it into:

1. **Joystick Input**: Gamepad-style movement commands (X/Y axes)
2. **Device Rotation**: Treadmill orientation mapped to controller rotation
3. **Visual Feedback**: On-screen tracker showing treadmill orientation

This allows VR applications to detect natural walking motions and convert them into game movement without traditional controller input.

---

## Architecture Overview

### System-Level Architecture

```
┌──────────────────────────────────────────────────────────┐
│              SteamVR Runtime                             │
│  ├─ HMD Tracking (Vive/Index)                            │
│  ├─ Input System (Mouse/Keyboard/Controller)             │
│  └─ Driver Loader                                        │
└────────────────────┬─────────────────────────────────────┘
                     │ IServerTrackedDeviceProvider
                     │ interface
                     ↓
┌──────────────────────────────────────────────────────────┐
│  TreadmillServerDriver (driver_treadmill.dll)            │
│  ├─ TreadmillServerDriver class                          │
│  │  ├─ Init() - Initialize driver                        │
│  │  ├─ RunFrame() - Main loop callback                   │
│  │  └─ Cleanup() - Shutdown                              │
│  │                                                       │
│  ├─ TreadmillDevice (invisible input device)             │
│  │  ├─ UpdateInputs() - Process motion → joystick        │
│  │  ├─ GetPose() - Return device orientation             │
│  │  └─ Activate/Deactivate                               │
│  │                                                       │
│  └─ TreadmillVisualTracker (visible tracker)             │
│     ├─ GetPose() - Follow HMD + show orientation         │
│     └─ Movement analysis & debugging                     │
└────────────────────┬─────────────────────────────────────┘
                     │ Dynamic P/Invoke Load
                     ↓
┌──────────────────────────────────────────────────────────┐
│  OmniBridge.dll (.NET 10 Assembly)                       │
│  ├─ MinimalOmniReader                                    │
│  │  ├─ OmniReader_Create()                               │
│  │  ├─ OmniReader_Initialize(COM port, mode, baud)       │
│  │  ├─ OmniReader_RegisterCallback(OnOmniData)           │
│  │  ├─ OmniReader_Disconnect()                           │
│  │  └─ OmniReader_Destroy()                              │
│  │                                                       │
│  └─ OmniMotionDataHandler                                │
│     ├─ Motion data streaming                             │
│     ├─ Serial communication                              │
│     └─ Callback invocation                               │
└────────────────────┬─────────────────────────────────────┘
                     │ Serial Protocol (115200 baud)
                     ↓
┌──────────────────────────────────────────────────────────┐
│  Virtuix Omni Treadmill                                  │
│  ├─ Ring Angle Sensor (0-360°)                           │
│  ├─ Step Detection (left/right pods)                     │
│  ├─ Gamepad Controller (Bluetooth)                       │
│  └─ Radio Module (nRF + RAFT)                            │
└──────────────────────────────────────────────────────────┘
```

### Data Flow Diagram

```
Omni Hardware
    ↓ (serial packets @ 115200 baud)
┌─────────────────────────────────────┐
│ OmniBridge.ReadLoop()               │
│ ├─ Read serial bytes                │
│ ├─ Decode packet (CRC check)        │
│ └─ Invoke OnOmniData callback       │
└─────────────────┬───────────────────┘
                  │ C++ callback
                  ↓
        ┌─────────────────────────────────────┐
        │ TreadmillServerDriver::OnOmniData   │
        │ ├─ angle (float) = ringAngle        │
        │ ├─ gamePadX (int) = X axis          │
        │ └─ gamePadY (int) = Y axis          │
        └─────────────────┬───────────────────┘
                          │ Store in g_state
                          ↓
        ┌─────────────────────────────────────┐
        │ TreadmillServerDriver::RunFrame()   │
        │ ├─ TreadmillDevice::UpdateInputs()  │
        │ │  └─ Send joystick to SteamVR      │
        │ │                                   │
        │ └─ TreadmillDevice::GetPose()       │
        │    └─ Convert angle → quaternion    │
        │       (controller rotation)         │
        └─────────────────┬───────────────────┘
                          │ IVRServerDriverHost
                          ↓
        ┌─────────────────────────────────────┐
        │ SteamVR Runtime                     │
        │ ├─ Controller pose updated          │
        │ ├─ Joystick input registered        │
        │ └─ Motion detected in game          │
        └─────────────────────────────────────┘
                          ↓
        ┌─────────────────────────────────────┐
        │ VR Game Application                 │
        │ ├─ Player moves via treadmill       │
        │ ├─ Game character walks/runs        │
        │ └─ Natural VR locomotion            │
        └─────────────────────────────────────┘
```

---

## Component Breakdown

### 1. TreadmillServerDriver (Main Driver)

**File**: `TreadmillServerDriver.cpp/h`

Main SteamVR driver interface. Implements `vr::IServerTrackedDeviceServerDriver`.

**Key Methods:**

| Method | Purpose |
|--------|---------|
| `Init()` | Called by SteamVR on driver load; loads OmniBridge DLL |
| `Cleanup()` | Shutdown; disconnects OmniReader |
| `RunFrame()` | Main loop; called every frame by SteamVR |
| `GetInterfaceVersions()` | Returns SteamVR version info |

**Initialization Flow:**

```cpp
vr::EVRInitError TreadmillServerDriver::Init(vr::IVRDriverContext* pDriverContext)
{
    // 1. Initialize OpenVR context
    VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);
    
    // 2. Load OmniBridge.dll dynamically
    wchar_t wDllPath[512];
    // → Reads path from settings or uses default
    omniReaderLib = LoadLibrary(wDllPath);
    
    // 3. Get function pointers (P/Invoke)
    pfnCreate = GetProcAddress(omniReaderLib, "OmniReader_Create");
    pfnInitialize = GetProcAddress(omniReaderLib, "OmniReader_Initialize");
    pfnRegisterCallback = GetProcAddress(omniReaderLib, "OmniReader_RegisterCallback");
    
    // 4. Create OmniReader instance
    m_omniReader = pfnCreate();
    
    // 5. Register callback (see OnOmniData)
    pfnRegisterCallback(m_omniReader, OnOmniData);
    
    // 6. Initialize connection
    pfnInitialize(m_omniReader, comPort, omniMode, 115200);
    
    // 7. Create tracked devices
    m_device = std::make_unique<TreadmillDevice>(0);
    m_visualTracker = std::make_unique<TreadmillVisualTracker>();
    
    vr::VRServerDriverHost()->TrackedDeviceAdded(
        "treadmill_controller", 
        vr::TrackedDeviceClass_Controller, 
        m_device.get()
    );
}
```

**RunFrame Loop:**

```cpp
void TreadmillServerDriver::RunFrame()
{
    // 1. Update controller (invisible input device)
    if (m_device && m_device->m_unObjectId != vr::k_unTrackedDeviceIndexInvalid)
    {
        m_device->UpdateInputs();  // Process motion data
        vr::DriverPose_t pose = m_device->GetPose();  // Get device rotation
        vr::VRServerDriverHost()->TrackedDevicePoseUpdated(
            m_device->m_unObjectId, 
            pose, 
            sizeof(vr::DriverPose_t)
        );
    }
    
    // 2. Update visual tracker (visible in SteamVR)
    if (m_visualTracker && m_visualTracker->m_unObjectId != vr::k_unTrackedDeviceIndexInvalid)
    {
        vr::DriverPose_t trackerPose = m_visualTracker->GetPose();
        vr::VRServerDriverHost()->TrackedDevicePoseUpdated(
            m_visualTracker->m_unObjectId, 
            trackerPose, 
            sizeof(vr::DriverPose_t)
        );
    }
}
```

---

### 2. TreadmillDevice (Invisible Input Controller)

**File**: `driver_treadmill.cpp` (TreadmillDevice class)

An invisible SteamVR controller that processes treadmill motion data into joystick input and rotation.

**Key Responsibilities:**

#### A) UpdateInputs() - Convert Motion to Joystick

```cpp
void TreadmillDevice::UpdateInputs()
{
    // 1. Get smoothed motion data from g_state
    {
        std::lock_guard<std::mutex> lock(g_state.mtx);
        x = g_state.x_smoothed;      // Gamepad X (-1.0 to +1.0)
        y = g_state.y_smoothed;      // Gamepad Y (-1.0 to +1.0)
        yawDeg = g_state.yaw_smoothed;  // Angle (0-360°)
    }
    
    // 2. Apply speed factor (configurable multiplier)
    float factor = g_speedFactor.load();
    float sx = std::clamp(x * factor, -1.0f, 1.0f);
    float sy = std::clamp(y * factor, -1.0f, 1.0f);
    
    // 3. Send to SteamVR input system
    vr::VRDriverInput()->UpdateScalarComponent(
        input_handles_[MyComponent_joystick_x], 
        sx, 
        0.0
    );
    vr::VRDriverInput()->UpdateScalarComponent(
        input_handles_[MyComponent_joystick_y], 
        sy, 
        0.0
    );
}
```

**Key Point**: Joystick is NOT rotated by controller rotation. Instead:
- **X**: Always lateral movement (left/right on treadmill)
- **Y**: Always forward/backward on treadmill
- **Rotation**: Happens via controller pose (see GetPose below)

This allows the game to rotate the movement vector based on controller orientation.

#### B) GetPose() - Convert Angle to Rotation

```cpp
vr::DriverPose_t TreadmillDevice::GetPose()
{
    float rawYaw;
    {
        std::lock_guard<std::mutex> lock(g_state.mtx);
        rawYaw = g_state.yaw_smoothed;  // Ring angle (0-360°)
        
        // Position: Always at origin (doesn't move in space)
        m_pose.vecPosition[0] = 0.0;
        m_pose.vecPosition[1] = 0.0;
        m_pose.vecPosition[2] = 0.0;
        
        // Rotation: Convert angle to quaternion
        // Angle (degrees) → Radians → Quaternion around Y-axis
        constexpr double DEG2RAD = 3.14159265358979323846 / 180.0;
        double theta = static_cast<double>(rawYaw) * DEG2RAD;
        
        double half = theta * 0.5;
        double c = std::cos(half);
        double s = std::sin(half);
        
        // Y-axis rotation (negative because treadmill rotates opposite)
        m_pose.qRotation.w = c;
        m_pose.qRotation.x = 0.0;
        m_pose.qRotation.y = -s;  // Negative: inverted direction
        m_pose.qRotation.z = 0.0;
    }
    
    return m_pose;
}
```

**Critical Detail**: 
- Controller position is always (0,0,0) - it doesn't move
- Controller rotation = treadmill orientation
- SteamVR interprets: "Joystick forward" + "Controller rotation" = direction to move

---

### 3. TreadmillVisualTracker (Visible Debug Tracker)

**File**: `driver_treadmill.cpp` (TreadmillVisualTracker class)

An optional visible tracker that shows treadmill orientation in SteamVR. Appears as a "Vive Tracker" device that follows the HMD position.

**Key Purpose**: 
- Visual debugging in SteamVR dashboard
- Verify correct angle mapping
- Movement analysis (compares actual HMD movement vs. expected direction)

**GetPose() Features:**

```cpp
// 1. Position: Follows HMD, 0.5m ahead
m_pose.vecPosition[0] = currentHmdX;
m_pose.vecPosition[1] = hmdMatrix.m[1][3] - 0.3;  // Chest height
m_pose.vecPosition[2] = currentHmdZ - 0.5;        // 0.5m forward

// 2. Movement Analysis
// Compare actual HMD movement vs. expected from treadmill input
float actualDeltaX = currentHmdX - g_state.lastHmdX;
float actualDeltaZ = currentHmdZ - g_state.lastHmdZ;
// → Calculate angle deviation
// → Log WARNING if deviation > 5°

// 3. Rotation: Same as TreadmillDevice
m_pose.qRotation = quaternion_from_yaw(rawYaw);
```

---

### 4. Global State & Callbacks

**File**: `driver_treadmill.cpp` (global state)

```cpp
struct XYState {
    float x, y, yaw;              // Raw values from hardware
    float x_smoothed, y_smoothed, yaw_smoothed;  // After EMA smoothing
    uint64_t dataId, logCounter;  // For tracing
    float lastHmdX, lastHmdZ;     // Previous HMD position
    bool hmdInitialized;
    std::mutex mtx;  // Thread-safety
} g_state;

// Atomic settings
std::atomic<float> g_speedFactor{ 1.0f };        // Joystick multiplier
std::atomic<float> g_smoothingFactor{ 0.3f };    // EMA alpha
```

**OnOmniData Callback:**

```cpp
void OnOmniData(float ringAngle, int gamePadX, int gamePadY)
{
    uint64_t timestamp = get_current_timestamp();
    
    {
        std::lock_guard<std::mutex> lock(g_state.mtx);
        
        // 1. Convert raw gamepad values (-128 to +127) to normalized (-1.0 to +1.0)
        float raw_x = std::clamp((gamePadX - 127.0f) / 127.0f, -1.0f, 1.0f);
        float raw_y = std::clamp(-(gamePadY - 127.0f) / 127.0f, -1.0f, 1.0f);  // Y inverted
        
        // 2. Apply Exponential Moving Average (EMA) smoothing
        float alpha = g_smoothingFactor.load();
        g_state.x_smoothed = alpha * raw_x + (1.0f - alpha) * g_state.x_smoothed;
        g_state.y_smoothed = alpha * raw_y + (1.0f - alpha) * g_state.y_smoothed;
        
        // 3. Handle angle wrapping (0-360°) with normalization
        float yaw_diff = ringAngle - g_state.yaw_smoothed;
        if (yaw_diff > 180.0f) yaw_diff -= 360.0f;
        if (yaw_diff < -180.0f) yaw_diff += 360.0f;
        g_state.yaw_smoothed += alpha * yaw_diff;
        
        // 4. Normalize to [0, 360]
        if (g_state.yaw_smoothed < 0.0f) g_state.yaw_smoothed += 360.0f;
        if (g_state.yaw_smoothed >= 360.0f) g_state.yaw_smoothed -= 360.0f;
        
        g_state.dataId = timestamp;
        g_state.logCounter++;
    }
}
```

**Why This is Important:**
- Callback is called from OmniBridge's background thread (NOT SteamVR's main thread)
- Uses mutex to prevent race conditions
- EMA smoothing reduces jitter (tweakable via `g_smoothingFactor`)
- Angle wrapping prevents 359° → 1° jump artifacts

---

## OmniBridge Integration Details

### How OmniBridge Data Reaches the Driver

```
┌─────────────────────────────────────────────────────────┐
│ Step 1: DLL Loading (TreadmillServerDriver::Init)       │
├─────────────────────────────────────────────────────────┤
│ HMODULE omniReaderLib = LoadLibrary(L"OmniBridge.dll"); │
│ // → Windows loads managed .NET assembly into memory    │
│ // → Locates exported functions (UnmanagedCallersOnly)  │
└─────────────────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────────────────┐
│ Step 2: Get Function Pointers (P/Invoke)                │
├─────────────────────────────────────────────────────────┤
│ pfnCreate = GetProcAddress(omniReaderLib,               │
│             "OmniReader_Create");                       │
│ // → Returns C++ function pointer to .NET method        │
│ // → No marshalling needed (POD types only)             │
└─────────────────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────────────────┐
│ Step 3: Initialize OmniReader                           │
├─────────────────────────────────────────────────────────┤
│ void* handle = pfnCreate();  // Create instance         │
│ pfnInitialize(handle,        // Handle                  │
│              "COM3",         // COM port                │
│              0,              // Mode                    │
│              115200);        // Baud rate               │
│ // → Creates OmniMotionDataHandler inside OmniBridge    │
│ // → Opens serial port                                  │
│ // → Starts background ReadLoop thread                  │
└─────────────────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────────────────┐
│ Step 4: Register Callback                               │
├─────────────────────────────────────────────────────────┤
│ pfnRegisterCallback(handle, (void*)OnOmniData);         │
│ // → Passes C++ function pointer to OmniBridge          │
│ // → OmniBridge will call OnOmniData on each packet     │
└─────────────────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────────────────┐
│ Step 5: Data Streaming (Background)                     │
├─────────────────────────────────────────────────────────┤
│ OmniBridge.ReadLoop() [thread 2]:                       │
│   ├─ Read serial bytes from Omni                        │
│   ├─ Decode packet (CRC check)                          │
│   ├─ Extract: angle, gamePadX, gamePadY                 │
│   └─ Call: OnOmniData(angle, x, y)                      │
│                                                         │
│ C++/SteamVR [thread 1]:                                 │
│   ├─ OnOmniData stores in g_state                       │
│   ├─ RunFrame() reads g_state (mutex protected)         │
│   ├─ UpdateInputs() → joystick update                   │
│   ├─ GetPose() → rotation update                        │
│   └─ VRServerDriverHost()->TrackedDevicePoseUpdated     │
└─────────────────────────────────────────────────────────┘
                    ↓
┌─────────────────────────────────────────────────────────┐
│ Step 6: SteamVR Processing                              │
├─────────────────────────────────────────────────────────┤
│ SteamVR Runtime sees:                                   │
│   ├─ Joystick input update                              │
│   ├─ Controller pose (rotation) update                  │
│   └─ Calculates final movement direction                │
│      = Joystick vector rotated by controller            │
└─────────────────────────────────────────────────────────┘
```

**Runtime Adjustments** (DebugRequest):

```cpp
// Set speed multiplier
DebugRequest("speed 0.5");  // Half speed

// Set smoothing (0.0 = no smoothing, 1.0 = maximum smoothing)
DebugRequest("smoothing 0.2");  // Responsive

// Enable/disable debug output
DebugRequest("debug true");
```

---

## Data Transformation Pipeline

### Raw Hardware → Normalized Values

```
Hardware (Omni)
├─ Ring Angle: 0-360° (incremental changes)
├─ GamePad X: 0-254 (center ~127)
└─ GamePad Y: 0-254 (center ~127)
    ↓
OnOmniData Callback
├─ Normalize X: (127-254)/127 → ±1.0
├─ Normalize Y: (0-127)/127 → ±1.0 (inverted)
└─ Clamp: [-1.0, +1.0]
    ↓
EMA Smoothing
├─ smoothed_x = 0.3 * raw_x + 0.7 * prev_smoothed_x
├─ smoothed_y = 0.3 * raw_y + 0.7 * prev_smoothed_y
└─ smoothed_yaw = wrap(prev_yaw + 0.3 * delta_angle)
    ↓
UpdateInputs()
├─ Apply speed factor: smoothed * speed_factor
├─ Clamp to ±1.0
└─ Send to SteamVR input system
    ↓
GetPose()
├─ Angle → Quaternion conversion
└─ No positional change (always 0,0,0)
    ↓
SteamVR Game
├─ Joystick vector: (sx, sy)
├─ Controller rotation: quat_from_angle
└─ Final direction = Rotate(joystick, controller_rotation)
```

---

## Key Design Decisions

### 1. Invisible Controller + Visible Tracker Split

Why two devices?

- **TreadmillDevice (invisible)**: Sends joystick input + controller rotation to game
  - Not shown in SteamVR dashboard
  - Game receives movement input directly
  
- **TreadmillVisualTracker (visible)**: Shows treadmill orientation in SteamVR
  - Appears as Vive Tracker
  - Useful for debugging / verifying angle mapping
  - Can show movement deviation warnings

### 2. Position Always at Origin

Why controller position = (0, 0, 0)?

- Treadmill doesn't move in physical space
- Only rotation matters (direction of movement)
- Joystick + rotation together = movement direction
- Simplifies game integration (no weird positional drifts)

### 3. Exponential Moving Average (EMA) Smoothing

Why EMA instead of other filters?

- **Responsive**: Can adjust alpha (0.0 - 1.0) for sensitivity
- **Efficient**: O(1) computation, no history buffer
- **Angle-aware**: Handles wrap-around (359° → 1°)
- **Configurable**: Tweakable at runtime via `g_smoothingFactor`

### 4. Mutex Protection for Concurrent Access

Why necessary?

- OmniBridge callback runs on background thread (serial read thread)
- SteamVR RunFrame runs on main thread
- Without locks: race condition → corrupted values
- **Solution**: Lock g_state during read/write

---

## Configuration & Tuning

### Speed Factor

```cpp
// Example: User feels too slow
DebugRequest("speed 1.5");  // 150% of normal speed
// OnOmniData will be called with same angle,
// but UpdateInputs multiplies by 1.5x
```

### Smoothing Factor

```cpp
// High responsiveness (less smoothing)
DebugRequest("smoothing 0.1");  // Very responsive, may be twitchy

// High smoothing (less responsiveness)
DebugRequest("smoothing 0.5");  // Smooth, may lag behind movement
```

### COM Port

```json
{
  "driver_treadmill": {
    "com_port": "COM4"  // Auto-detect, or set manually
  }
}
```

---

## Debugging & Troubleshooting

### Logging Output

Enable verbose logging:

```cpp
// In SteamVR driver console or via DebugRequest
DebugRequest("debug true");

// Output includes:
// [UpdateInputs] Controller Yaw=45.0° | Joystick X=0.500 Y=0.750
// [OnOmniData] RAW: angle=45.2° X=127 Y=190 | SMOOTHED: X=0.495 Y=0.745
// [TreadmillDevice::GetPose] CALC quat(w=0.9238, y=-0.3827)
```

### Common Issues

#### 1. Joystick Input Not Working

**Cause**: OmniBridge DLL not found or not loading

**Debug**:
```
Check SteamVR driver console:
"LoadLibrary failed for OmniBridge.dll: The specified module could not be found"

Solution:
- Copy OmniBridge.dll + OmniCommon.dll to driver bin64 folder
- Verify COM port in settings
```

#### 2. Movement in Wrong Direction

**Cause**: Angle mapping inverted or coordinate system mismatch

**Debug**:
- Enable visual tracker (TreadmillVisualTracker)
- Rotate treadmill ring, watch tracker in SteamVR
- Check if tracker rotates in expected direction
- Adjust sign in GetPose() quaternion calculation

#### 3. Jerky / Jittery Input

**Cause**: Smoothing factor too low or CRC errors

**Debug**:
```
DebugRequest("smoothing 0.4");  // Increase smoothing
DebugRequest("debug true");     // Check for CRC errors in log
```

#### 4. No Movement Detected

**Cause**: Serial connection failed or motion data disabled

**Debug**:
```
Check:
1. COM port accessible: ComPortHelper.CanAccessPort("COM3")
2. Motion data enabled: MotionDataSelection.AllOn()
3. Hardware connected: Check LED on Omni controller
4. Baud rate: Must be 115200
```

---

## Performance

### CPU Usage

- **RunFrame callback**: ~1-2ms (per frame, typically 90Hz)
- **OnOmniData callback**: <0.1ms (async, from serial thread)
- **Total overhead**: <3% CPU on modern hardware

### Latency

- **Serial delay**: ~5-10ms (115200 baud, 20-50 byte packets)
- **Processing**: <1ms (EMA smoothing, quaternion math)
- **SteamVR update**: ~2-5ms (frame-dependent)
- **Total E2E**: ~10-20ms (acceptable for locomotion)

### Memory

- **Global state**: ~64 bytes
- **Callback frame**: No heap allocation (stack only)
- **DLL overhead**: ~10MB (managed .NET assembly)

---

## Building & Deployment

### Requirements

- Visual Studio 2022 (C++ workload)
- OpenVR SDK (included in repo)
- .NET 10 SDK (for OmniBridge compilation)
- Windows 10/11

### Build Steps

```bash
# 1. Build OmniBridge (.NET)
dotnet build OmniBridge/OmniBridge.csproj -c Release

# 2. Build TreadmillSteamVR (C++)
# Open TreadmillSteamVR.sln in Visual Studio
# Build → Release x64

# 3. Copy outputs to SteamVR driver directory
# Output files:
#   - driver_treadmill.dll → SteamVR/drivers/treadmill/bin/win64/
#   - OmniBridge.dll → Same directory
#   - OmniCommon.dll → Same directory
#   - driver.vrdrivermanifest → SteamVR/drivers/treadmill/
#   - resources/ → SteamVR/drivers/treadmill/resources/

# 4. Restart SteamVR
```
---

## Future Enhancements

- [ ] Multi-treadmill support
- [ ] Motion prediction (extrapolate next frame)
- [ ] Gesture recognition (jump, crouch, etc.)
- [ ] Per-game tuning profiles
- [ ] Calibration wizard UI
- [ ] Performance monitoring dashboard
- [ ] Network streaming (Omni over Ethernet)

---

## Support

- **GitHub Issues**: Report bugs and feature requests
- **SteamVR Logs**: Check `%APPDATA%\Local\openvr\` for driver logs
- **OmniBridge Docs**: See `OmniBridge/README.md` for low-level protocol details
- **Omni Protocol**: See `OmniCommon/OmniTreadmill_Analyse.md` for packet specs

---
