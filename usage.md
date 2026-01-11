# TreadmillSteamVR Driver - Installation & Usage Guide

Complete guide for installing and setting up the Omni Treadmill SteamVR driver on your system.

---

## Quick Start

1. **Build the projects**:
   ```bash
   # Build OmniBridge (.NET 10)
   dotnet build OmniBridge/OmniBridge.csproj -c Release
   
   # Build TreadmillSteamVR (C++, in Visual Studio)
   # Open TreadmillSteamVR.sln → Build → Release x64
   ```

2. **Copy files to SteamVR directory** (see section below)

3. **Restart SteamVR**

4. **Check console for "Successfully loaded driver treadmill"**

---

## Directory Structure Overview

### SteamVR Installation Directory Structure

```
C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers\treadmill\
├── bin/
│   └── win64/
│       ├── driver_treadmill.dll         # ← COPY HERE (built C++ DLL)
│       ├── OmniBridge.dll               # ← COPY HERE
│       └── OmniCommon.dll               # ← COPY HERE
│
├── resources/
│   ├── settings/
│   │   └── default.vrsettings           # ← COPY HERE
│   ├── input/
│   │   ├── bindings_treadmill_controller.json
│   │   ├── legacy_bindings.json
│   │   ├── steam.app.611670.json
│   │   ├── treadmill_profile.json
│   └── icons/
│       └── treadmill.svg
│
└── driver.vrdrivermanifest              # ← COPY HERE
```

---

## Installation Steps

### Step 1: Build OmniBridge (.NET 10 Assembly)

**Purpose**: Creates the managed wrapper around Omni Treadmill communication.

```bash
cd C:\Users\<Username>\Documents\GitHub\Omni-Pro-Treadmill-Driver\

# Build release version
dotnet build OmniBridge/OmniBridge.csproj -c Release -o OmniBridge/bin/Release/net10.0/win-x64/
```

**Output Files**:
- `OmniBridge/bin/Release/net10.0/win-x64/OmniBridge.dll` ← Contains MinimalOmniReader exports
- `OmniBridge/bin/Release/net10.0/win-x64/OmniCommon.dll` ← Contains OmniPacketBuilder, etc.

**What It Does**:
- Opens serial connection to Omni Treadmill (COM3, 115200 baud)
- Decodes motion packets (ring angle, gamepad input)
- Exposes C-style exports for P/Invoke (OmniReader_Create, etc.)
- Runs background thread for serial communication

---

### Step 2: Build TreadmillSteamVR (C++ Driver)

**Purpose**: Creates the native SteamVR driver that integrates with the VR runtime.

**Steps**:
1. Open `TreadmillSteamVR.sln` in Visual Studio
2. Select "Release" configuration
3. Select "x64" platform
4. Build → Build Solution
5. Wait for compilation to complete

**Output File**:
- `x64/Release/driver_treadmill.dll` ← Main driver executable

**What It Does**:
- Loads OmniBridge.dll dynamically (P/Invoke)
- Registers two tracked devices with SteamVR:
  - **TreadmillDevice** (invisible): Sends joystick input + controller rotation
  - **TreadmillVisualTracker** (visible): Shows treadmill orientation in SteamVR dashboard
- Processes motion data in background (OnOmniData callback)
- Applies smoothing and speed factors

---

### Step 3: Create SteamVR Driver Directory

**If the directory doesn't exist**:

```bash
# Create the treadmill driver directory structure
mkdir "C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers\treadmill"
mkdir "C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers\treadmill\bin"
mkdir "C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers\treadmill\bin\win64"
mkdir "C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers\treadmill\resources"
mkdir "C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers\treadmill\resources\settings"
mkdir "C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers\treadmill\resources\input"
mkdir "C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers\treadmill\resources\icons"
```

---

### Step 4: Copy DLL Files

**Copy the compiled binaries**:

```bash
# Copy C++ driver DLL
copy "C:\Users\<Username>\Documents\GitHub\Omni-Pro-Treadmill-Driver\x64\Release\driver_treadmill.dll" ^
     "C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers\treadmill\bin\win64\"

# Copy OmniBridge.dll
copy "C:\Users\<Username>\Documents\GitHub\Omni-Pro-Treadmill-Driver\OmniBridge\bin\Release\net10.0\win-x64\OmniBridge.dll" ^
     "C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers\treadmill\bin\win64\"

# Copy OmniCommon.dll (dependency)
copy "C:\Users\<Username>\Documents\GitHub\Omni-Pro-Treadmill-Driver\OmniBridge\bin\Release\net10.0\win-x64\OmniCommon.dll" ^
     "C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers\treadmill\bin\win64\"
```

**Verify**: All three DLLs should be in `bin\win64\`:
- ✅ driver_treadmill.dll (C++ driver)
- ✅ OmniBridge.dll (managed wrapper)
- ✅ OmniCommon.dll (protocol library)

---

### Step 5: Copy Configuration Files

**Copy settings and input bindings**:

```bash
# Copy default settings
copy "C:\Users\<Username>\Documents\GitHub\Omni-Pro-Treadmill-Driver\treadmill\resources\settings\default.vrsettings" ^
     "C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers\treadmill\resources\settings\"

# Copy all input binding files
copy "C:\Users\<Username>\Documents\GitHub\Omni-Pro-Treadmill-Driver\treadmill\resources\input\*.json" ^
     "C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers\treadmill\resources\input\"

# Copy icons
copy "C:\Users\<Username>\Documents\GitHub\Omni-Pro-Treadmill-Driver\treadmill\resources\icons\treadmill.svg" ^
     "C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers\treadmill\resources\icons\"
```

---

### Step 6: Copy Manifest File

**Copy the driver manifest**:

```bash
copy "C:\Users\<Username>\Documents\GitHub\Omni-Pro-Treadmill-Driver\driver.vrdrivermanifest" ^
     "C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers\treadmill\"
```

**What It Contains**:
- Driver name: "Treadmill"
- Driver version
- Entry point: "driver_treadmill.dll"
- Supported platforms: "win64"

---

### Step 7: Restart SteamVR

```bash
# Close SteamVR completely
taskkill /IM vrserver.exe /F
taskkill /IM vrdashboard.exe /F

# Relaunch SteamVR
# SteamVR will detect the new driver and load it automatically
```

**Verification**:
1. Open SteamVR console
2. Look for: `"Successfully loaded driver treadmill"`
3. Check devices list for:
   - "Treadmill Controller" (invisible)
   - "Treadmill Visual Tracker" (visible, appears as Vive Tracker)

---

## File Descriptions

### Binary Files (Compiled)

#### `driver_treadmill.dll` (C++)
| Aspect | Details |
|--------|---------|
| **Purpose** | Main SteamVR driver interface |
| **Built from** | driver_treadmill.cpp, TreadmillServerDriver.cpp, TreadmillDevice.h |
| **Exports** | HmdDriverFactory (SteamVR entry point) |
| **Dependencies** | OpenVR SDK, Windows API, OmniBridge.dll |
| **Size** | ~200-300 KB (release build) |
| **Function** | Initializes driver, manages motion data, updates SteamVR poses |

**When Used**:
- SteamVR loads on startup
- On demand by VR applications
- During device pose updates (every frame)

**What It Does**:
```
Load → Find OmniBridge.dll → Initialize → Register devices →
Loop: Read motion data → Convert to joystick/rotation →
      Update SteamVR poses → Process game commands
```

---

#### `OmniBridge.dll` (.NET 10)
| Aspect | Details |
|--------|---------|
| **Purpose** | Bridge between C++ driver and Omni Treadmill |
| **Built from** | OmniBridge project (MinimalOmniReader, OmniMotionDataHandler, ComPortHelper) |
| **Exports** | OmniReader_Create/Initialize/RegisterCallback/Disconnect/Destroy |
| **Dependencies** | OmniCommon.dll, .NET 10 Runtime |
| **Size** | ~15-20 KB (DLL only) |
| **Function** | Serial communication, packet decoding, callback management |

**When Used**:
- Loaded by driver_treadmill.dll at startup (LoadLibrary)
- Background thread runs continuously during gameplay

**What It Does**:
```
Open COM port (115200 baud) → Read serial packets → Decode (CRC check) →
Extract motion data (angle, gamepad) → Invoke C++ callback →
Continue loop (10ms sleep)
```

---

#### `OmniCommon.dll`
| Aspect | Details |
|--------|---------|
| **Purpose** | Omni protocol library (decompiled) |
| **Built from** | OmniCommon project (OmniPacketBuilder, Message types, etc.) |
| **Exports** | None (linked statically by OmniBridge) |
| **Dependencies** | .NET 10 Runtime |
| **Size** | ~100-150 KB |
| **Function** | Packet encoding/decoding, CRC16 checksum, message parsing |

**When Used**:
- Referenced by OmniBridge.dll
- Used for every serial packet received

**What It Does**:
```
Receive byte buffer → Validate frame markers (0xEF, 0xBE) →
Check CRC16 checksum → Extract packet type →
Decode payload (motion data, raw sensor data, etc.) →
Return OmniMotionData object
```

---

### Configuration Files

#### `default.vrsettings`
| Aspect | Details |
|--------|---------|
| **Path** | `treadmill\resources\settings\default.vrsettings` |
| **Format** | JSON |
| **Purpose** | Driver configuration (COM port, speed factor, smoothing) |
| **When Read** | When driver initializes (once per SteamVR startup) |

**Key Settings**:
```json
{
  "driver_treadmill": {
    "com_port": "COM3",                    // Omni Treadmill serial port
    "omnibridge_dll_path": "...",         // Path to OmniBridge.dll
    "speed_factor": 1.0,                  // Joystick speed multiplier
    "smoothing_factor": 0.3,              // EMA smoothing (0.0-1.0)
    "debug": true                         // Enable verbose logging
  }
}
```

**How to Modify**:
- Edit directly in text editor
- Restart SteamVR to apply changes
- Or use DebugRequest API at runtime

---

#### `treadmill_profile.json`
| Aspect | Details |
|--------|---------|
| **Path** | `treadmill\resources\input\treadmill_profile.json` |
| **Format** | SteamVR input profile JSON |
| **Purpose** | Defines joystick axes and mapping |
| **When Used** | SteamVR startup (loads input profile) |

**Key Components**:
```json
{
  "/input/joystick/x": "Joystick X-axis (lateral movement)",
  "/input/joystick/y": "Joystick Y-axis (forward/backward)"
}
```

**What It Does**:
- Tells SteamVR what input components are available
- Maps treadmill joystick to game controllers
- Defines range: -1.0 (left/backward) to +1.0 (right/forward)

---

#### `bindings_treadmill_controller.json` (and other binding files)
| Aspect | Details |
|--------|---------|
| **Path** | `treadmill\resources\input\bindings_*.json` |
| **Format** | SteamVR binding definition |
| **Purpose** | Game-specific input mapping |
| **When Used** | When launching game with SteamVR |

**Example Mapping**:
```json
{
  "treadmill_controller": {
    "/input/joystick": {
      "output": "/actions/locomotion/in/move"  // Game's movement action
    }
  }
}
```

**What It Does**:
- Connects treadmill joystick to game actions
- Allows multiple games to use same physical device
- Each game can have custom sensitivity settings

---

#### `driver.vrdrivermanifest`
| Aspect | Details |
|--------|---------|
| **Path** | `treadmill\driver.vrdrivermanifest` |
| **Format** | JSON manifest |
| **Purpose** | Driver metadata (name, version, entry point) |
| **When Used** | SteamVR initialization (during driver discovery) |

**Contents**:
```json
{
  "name": "Treadmill",
  "version": "1.0.0",
  "driver": "bin/win64/driver_treadmill.dll",
  "supported_platforms": [ "win64" ]
}
```

**What SteamVR Does**:
1. Scans `drivers/` directory for manifest files
2. Reads driver name and DLL path
3. Loads DLL and calls HmdDriverFactory
4. Registers driver with SteamVR runtime

---

### Source Code Files (Reference)

#### `driver_treadmill.cpp`
| Aspect | Details |
|--------|---------|
| **Purpose** | Main driver implementation (OnOmniData callback, device management) |
| **Size** | ~800 lines |
| **Key Functions** | OnOmniData (serial callback), Log (debug output), global state management |

**Flow**:
```
SteamVR calls HmdDriverFactory
  ↓
TreadmillServerDriver::Init()
  ├─ Load OmniBridge.dll
  ├─ Register OnOmniData callback
  └─ Create TreadmillDevice
  
OnOmniData receives packet (every ~10ms from serial)
  ├─ Extract angle, gamePadX, gamePadY
  ├─ Apply EMA smoothing
  ├─ Store in global g_state (mutex protected)
  
RunFrame called by SteamVR (90 Hz)
  ├─ Read g_state (mutex protected)
  ├─ UpdateInputs() → Send joystick to SteamVR
  └─ GetPose() → Send device rotation
```

---

#### `TreadmillServerDriver.cpp/h`
| Aspect | Details |
|--------|---------|
| **Purpose** | SteamVR driver interface implementation |
| **Implements** | vr::IServerTrackedDeviceProvider |
| **Key Methods** | Init(), Cleanup(), RunFrame() |

---

#### `TreadmillDevice.h`
| Aspect | Details |
|--------|---------|
| **Purpose** | Defines device classes (TreadmillDevice, TreadmillVisualTracker) |
| **Classes** | TreadmillDevice (invisible), TreadmillVisualTracker (visible) |

---

#### `MinimalOmniReader.h`
| Aspect | Details |
|--------|---------|
| **Purpose** | Unmanaged exports for P/Invoke |
| **Exports** | OmniReader_Create, Initialize, RegisterCallback, etc. |

---

## Runtime Flow Diagram

```
┌─────────────────────────────────────────────────────────┐
│ 1. SteamVR Startup                                      │
│    ├─ Scan drivers/ directory                          │
│    ├─ Find driver.vrdrivermanifest                     │
│    └─ Load bin/win64/driver_treadmill.dll              │
└────────────────┬────────────────────────────────────────┘
                 ↓
┌─────────────────────────────────────────────────────────┐
│ 2. Driver Initialization (TreadmillServerDriver::Init) │
│    ├─ VR_INIT_SERVER_DRIVER_CONTEXT                    │
│    ├─ LoadLibrary("OmniBridge.dll")                    │
│    ├─ Get function pointers (P/Invoke)                │
│    ├─ pfnCreate() → create OmniReader instance        │
│    ├─ pfnRegisterCallback(OnOmniData)                 │
│    ├─ pfnInitialize() → open COM3 @ 115200           │
│    └─ Register tracked devices (TreadmillDevice)      │
└────────────────┬────────────────────────────────────────┘
                 ↓
┌─────────────────────────────────────────────────────────┐
│ 3. Serial Communication (OmniBridge background thread) │
│    Loop:                                                │
│    ├─ Read bytes from COM port                        │
│    ├─ Decode packet (CRC check)                       │
│    ├─ Call OnOmniData(angle, x, y)                   │
│    ├─ Store in g_state (mutex)                        │
│    └─ Sleep 10ms                                      │
└────────────────┬────────────────────────────────────────┘
                 ↓ (parallel)
┌─────────────────────────────────────────────────────────┐
│ 4. SteamVR Main Loop (90 Hz)                           │
│    Loop:                                                │
│    ├─ TreadmillServerDriver::RunFrame()               │
│    ├─ Read g_state (mutex)                            │
│    ├─ TreadmillDevice::UpdateInputs()                 │
│    │  └─ VRDriverInput()->UpdateScalarComponent()    │
│    ├─ TreadmillDevice::GetPose()                      │
│    │  └─ Convert angle → quaternion                  │
│    ├─ VRServerDriverHost()->TrackedDevicePoseUpdated()
│    └─ Return to SteamVR                              │
└────────────────┬────────────────────────────────────────┘
                 ↓
┌─────────────────────────────────────────────────────────┐
│ 5. VR Game Processing                                  │
│    ├─ Read joystick input (X, Y)                      │
│    ├─ Read controller rotation (yaw)                  │
│    ├─ Calculate movement direction                    │
│    │  = Joystick rotated by controller rotation       │
│    ├─ Apply to player movement                        │
│    └─ Continue game loop                              │
└─────────────────────────────────────────────────────────┘
```

---

## Troubleshooting Installation

### Issue: "Driver failed to load"

**Cause**: Missing DLL files or wrong path

**Solution**:
```bash
# Verify all files exist in bin\win64\
dir "C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers\treadmill\bin\win64\"

# Should show:
# - driver_treadmill.dll
# - OmniBridge.dll
# - OmniCommon.dll
```

---

### Issue: "Could not find COM port"

**Cause**: Default COM3 not available

**Solution**:
1. Connect Omni Treadmill
2. Check Device Manager for actual COM port
3. Edit `default.vrsettings`:
   ```json
   "com_port": "COM4"   // Change to correct port
   ```
4. Restart SteamVR

---

### Issue: "LoadLibrary failed for OmniBridge.dll"

**Cause**: .NET 10 Runtime not installed or path wrong

**Solution**:
1. Install .NET 10 SDK: https://dotnet.microsoft.com/download/dotnet/10.0
2. Verify OmniBridge.dll exists and is x64
3. Check `default.vrsettings` path

---

### Issue: "Motion data not received"

**Cause**: Serial communication failed or no motion data enabled

**Solution**:
```json
// In default.vrsettings
"debug": true    // Enable logging

// Check SteamVR console for errors
// Verify Omni Treadmill is connected and powered
// Try different COM port
```

---

## File Update Workflow

When you update the code:

```bash
# 1. Make code changes in repo
# 2. Rebuild locally
dotnet build OmniBridge/OmniBridge.csproj -c Release
# (rebuild C++ in Visual Studio)

# 3. Copy new files to SteamVR directory
xcopy /Y "C:\Users\<Username>\...\OmniBridge\bin\Release\net10.0\win-x64\*.dll" ^
       "C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers\treadmill\bin\win64\"

xcopy /Y "C:\Users\<Username>\...\x64\Release\driver_treadmill.dll" ^
       "C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers\treadmill\bin\win64\"

# 4. Restart SteamVR
taskkill /IM vrserver.exe /F
# Relaunch SteamVR
```

---

## Summary Table

| File | Source | Destination | Type | When Used |
|------|--------|-------------|------|-----------|
| driver_treadmill.dll | x64/Release/ | bin/win64/ | C++ DLL | SteamVR startup |
| OmniBridge.dll | OmniBridge/bin/Release/ | bin/win64/ | .NET DLL | Driver init |
| OmniCommon.dll | OmniBridge/bin/Release/ | bin/win64/ | .NET DLL | Packet decoding |
| default.vrsettings | treadmill/resources/ | resources/settings/ | Config | Driver init |
| *.json (input) | treadmill/resources/input/ | resources/input/ | Config | Game startup |
| driver.vrdrivermanifest | Repository root | treadmill/ | Manifest | SteamVR startup |
| treadmill.svg | treadmill/resources/icons/ | resources/icons/ | Icon | SteamVR dashboard |

---

## Next Steps

1. ✅ Install files (this guide)
2. ✅ Verify files are in correct locations
3. ✅ Restart SteamVR
4. ✅ Check SteamVR console for "Successfully loaded driver treadmill"
5. ⏭️ Launch VR game and test motion input
6. ⏭️ Adjust speed_factor and smoothing_factor in settings

See `README.md` for more details on architecture and configuration options.

