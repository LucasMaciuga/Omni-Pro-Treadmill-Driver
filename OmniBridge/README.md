# OmniBridge

A .NET 10 interop library that bridges Virtuix Omni Treadmill motion data to native C++ applications via P/Invoke and dynamic library loading.

## Overview

**OmniBridge** provides a managed wrapper around the OmniCommon library, exposing treadmill sensor data (ring angle, gamepad input, motion vectors) to native C++ code through unmanaged callback functions. It handles serial communication initialization, packet encoding/decoding, and thread-safe data streaming.

### Key Features

- **Serial Port Communication**: Auto-detects and connects to Omni Treadmill via configurable COM port
- **Motion Data Streaming**: Captures ring angle, step count, gamepad input, and raw pod sensor data
- **P/Invoke Integration**: Exports native C++ entry points for dynamic loading (`OmniReader_Create`, `OmniReader_Initialize`, etc.)
- **Callback Architecture**: Streams processed motion data to native code via registered C++ callbacks
- **Thread-Safe Operations**: Mutex-protected state management for concurrent reads/writes
- **Configurable Output**: Selective data streaming (timestamp, steps, angle, gamepad, etc.)

---

## Architecture

### Component Hierarchy

```
???????????????????????????????????????????
?     Native C++ Application              ?
?  (TreadmillServerDriver - SteamVR)      ?
???????????????????????????????????????????
             ? P/Invoke Dynamic Load
             ?
???????????????????????????????????????????
?  OmniBridge.dll (.NET 10 Assembly)      ?
?  ?? MinimalOmniReader                   ?
?  ?  ?? Create/Initialize/Destroy        ?
?  ?  ?? RegisterCallback                 ?
?  ?  ?? Disconnect                       ?
?  ?? OmniMotionDataHandler               ?
?     ?? SerialPort Management            ?
?     ?? Packet Encoding/Decoding         ?
?     ?? Motion Data Streaming            ?
?     ?? Event System                     ?
???????????????????????????????????????????
             ? Serial Protocol
             ?
???????????????????????????????????????????
?  OmniCommon Library (Decompiled)        ?
?  ?? OmniPacketBuilder                   ?
?  ?? Message Types                       ?
?  ?? CRC16 Checksum                      ?
?  ?? Motion Data Parsing                 ?
???????????????????????????????????????????
             ? USB Serial
             ?
???????????????????????????????????????????
?  Virtuix Omni Treadmill Hardware        ?
?  ?? Motion Ring Sensor (Angle)          ?
?  ?? Step Detection Pods (Feet)          ?
?  ?? Bluetooth Gamepad Controller        ?
?  ?? Radio Module (NRF + RAFT)           ?
???????????????????????????????????????????
```

### Main Classes

#### `MinimalOmniReader` (Unmanaged Exports)

Provides the C++ interop interface via `[UnmanagedCallersOnly]` entry points:

| Method | Purpose | Parameters | Returns |
|--------|---------|------------|---------|
| `OmniReader_Create()` | Create reader instance | — | `nint` handle |
| `OmniReader_Initialize()` | Connect and configure | COM port, mode, baudrate | `bool` success |
| `OmniReader_RegisterCallback()` | Register data callback | handle, callback ptr | — |
| `OmniReader_Disconnect()` | Stop streaming | handle | — |
| `OmniReader_Destroy()` | Clean up resources | handle | — |

**Callback Signature:**
```cpp
typedef void (*OmniDataCallback)(float ringAngle, int gamePadX, int gamePadY);
```

#### `OmniMotionDataHandler` (Core Logic)

Manages the serial connection and data streaming:

- **Connect()**: Opens COM port, sends configuration, starts reader thread
- **SetMode()**: Changes movement mode (e.g., `ForwardBackStrafe`)
- **ConfigureMotionData()**: Selects which motion fields to stream
- **Disconnect()**: Gracefully shuts down connection
- **ReadLoop()**: Background thread processing incoming packets
- **ProcessMessage()**: Decodes received OmniBaseMessage objects

**Key Properties:**
- `IsConnected`: Serial port open state
- `CurrentMode`: Active movement mode (OmniMode enum)
- `CrcErrorCount`: Tracks malformed packets

**Events:**
- `MotionDataReceived`: Fired when motion data arrives
- `RawHexDataReceived`: Raw packet bytes (for debugging)
- `ConnectionError`: Serial/hardware failures
- `ModeChanged`: OmniMode changes
- `CrcErrorOccurred`: Checksum validation failures

#### `ComPortHelper` (Utilities)

Serial port discovery and management:

- `GetAvailablePorts()`: List system COM ports
- `IsPortAvailable()`: Check if port exists
- `CanAccessPort()`: Test port accessibility (tries to open/close)
- `DisplayAvailablePorts()`: Console output with accessibility
- `SelectPortInteractive()`: User-guided port selection
- `FindFirstAccessiblePort()`: Auto-detection

---

## Usage

### Basic Connection Example

```csharp
using OmniCommon;
using OmniCommon.Messages;

// 1. Create handler
var handler = new OmniMotionDataHandler(
    comPort: "COM3",
    baudRate: 115200,
    configurationDelayMs: 100,
    initialMode: OmniMode.ForwardBackStrafe
);

// 2. Register event listener
handler.MotionDataReceived += (sender, data) =>
{
    Console.WriteLine($"Angle: {data.RingAngle:F1}°");
    Console.WriteLine($"Steps: {data.StepCount}");
    Console.WriteLine($"Gamepad: X={data.GamePad_X}, Y={data.GamePad_Y}");
};

// 3. Connect with custom configuration
var motionSelection = new MotionDataSelection
{
    Timestamp = true,
    StepCount = true,
    RingAngle = true,    // Required for SteamVR driver
    GamePadData = true,
    RingDelta = false,
    GunButtonData = false,
    StepTrigger = true
};

if (handler.Connect(motionSelection, OmniMode.ForwardBackStrafe))
{
    Console.WriteLine($"Connected to {handler.PortName}");
    
    // 4. Process data (streaming in background thread)
    System.Threading.Thread.Sleep(5000);
    
    // 5. Disconnect
    handler.Disconnect();
}
else
{
    Console.WriteLine("Connection failed");
}
```

### P/Invoke Usage (C++)

```cpp
// Load the DLL
HMODULE lib = LoadLibraryW(L"OmniBridge.dll");

// Get function pointers
auto pfnCreate = (void* (*)(void))GetProcAddress(lib, "OmniReader_Create");
auto pfnInitialize = (bool (*)(void*, const char*, int, int))GetProcAddress(lib, "OmniReader_Initialize");
auto pfnRegisterCallback = (void (*)(void*, void*))GetProcAddress(lib, "OmniReader_RegisterCallback");

// Create instance
void* handle = pfnCreate();

// Define callback
void OmniCallback(float angle, int x, int y) {
    printf("Angle: %.1f, Gamepad: (%d, %d)\n", angle, x, y);
}

// Initialize
pfnRegisterCallback(handle, (void*)OmniCallback);
if (pfnInitialize(handle, "COM3", 0, 115200)) {
    // Streaming...
    Sleep(5000);
}

// Cleanup
pfnDisconnect(handle);
pfnDestroy(handle);
FreeLibrary(lib);
```

---

## Data Flow

### 1. Initialization

```
User Code
    ?
OmniMotionDataHandler.Connect()
    ?? Open SerialPort (COM3, 115200 baud)
    ?? Load settings (port name, mode, delays)
    ?? Send OmniSetMotionDataMessage (configure output)
    ?? Wait 100ms (hardware reaction time)
    ?? Start ReadLoop thread
    ?? Return success
```

### 2. Data Reception & Processing

```
Hardware (Omni Treadmill)
    ? (115200 baud serial stream)
ReadLoop Thread
    ?? Read bytes into buffer
    ?? OmniPacketBuilder.decodePacket()
    ?  ?? Validate start marker (0xEF)
    ?  ?? Check CRC16 checksum
    ?  ?? Extract payload
    ?? Determine message type
    ?  ?? OmniMotionDataMessage
    ?  ?  ?? Extract: timestamp, steps, angle, gamepad
    ?  ?? OmniRawDataMessage
    ?  ?  ?? Extract: pod quaternions, accel, gyro
    ?  ?? Other types (ignored)
    ?? Raise MotionDataReceived event
    ?? Loop (10ms sleep to reduce CPU)
```

### 3. P/Invoke Callback

```
MotionDataReceived Event
    ?
Native Code receives callback
    ?
TreadmillServerDriver processes angle ? device rotation
    ?
SteamVR Driver updates tracker pose
    ?
VR Application sees treadmill orientation
```

---

## Configuration

### Serial Connection Settings

| Parameter | Default | Description |
|-----------|---------|-------------|
| `comPort` | "COM3" | Serial port identifier |
| `baudRate` | 115200 | Baud rate (fixed for Omni) |
| `configurationDelayMs` | 100 | Delay after config message (ms) |
| `initialMode` | ForwardBackStrafe | Movement mode |

### Motion Data Selection

```csharp
var selection = MotionDataSelection.AllOn();  // Enable everything

// Or selective configuration:
selection.Timestamp = true;
selection.StepCount = true;
selection.RingAngle = true;      // CRITICAL: Ring angle for tracking
selection.GamePadData = true;
selection.RingDelta = false;
selection.GunButtonData = false;
selection.StepTrigger = true;
```

### OmniMode Enum

Defines treadmill movement interpretation:

- **ForwardBackStrafe**: Standard omnidirectional (used by SteamVR driver)
- **DecoupledForwardBackStrafe**: Alternative interpretation
- **Other modes**: Additional variants for different applications

---

## Error Handling

### Common Issues & Solutions

#### No Data Received

```csharp
if (!handler.IsConnected)
{
    Console.WriteLine("Port not open!");
    return;
}

if (handler.CrcErrorCount > 0)
{
    Console.WriteLine($"CRC errors detected: {handler.CrcErrorCount}");
    handler.Disconnect();
    handler.Connect();  // Reconnect
}
```

#### Port Access Denied

```csharp
if (!ComPortHelper.CanAccessPort("COM3", 115200))
{
    Console.WriteLine("Port in use by another application");
    var port = ComPortHelper.SelectPortInteractive();
    if (port != null)
        handler = new OmniMotionDataHandler(port);
}
```

#### Timeout During Connection

```csharp
try
{
    if (!handler.Connect(selection, mode))
    {
        // Hardware didn't respond
        Console.WriteLine("Timeout: Check USB connection");
    }
}
catch (UnauthorizedAccessException)
{
    Console.WriteLine("Insufficient permissions");
}
```

---

## Thread Safety

OmniBridge uses **mutex-protected state** to safely handle concurrent access:

```csharp
private readonly object _lockObject = new();

// In Connect() and ReadLoop():
lock (_lockObject)
{
    _port.Write(packet, 0, packet.Length);  // Send command
    bytesRead = _port.Read(buffer, 0, buffer.Length);  // Receive
}
```

**Important:** Do NOT call `Connect()`/`Disconnect()` from the motion data callback—instead, use a signal or event to trigger these operations from the main thread.

---

## Performance Considerations

### CPU Usage

- **Read Loop**: 10ms sleep between iterations ? ~100 checks/second
- **Low overhead** for serial reads if data available
- **Minimal** if no motion data flowing

### Memory

- **Buffer**: 256-byte read buffer (reused)
- **Callback**: Native code responsibility; OmniBridge just forwards
- **Events**: Standard .NET event system (minimal allocation)

### Latency

- **Serial: ~1-10ms** per packet (115200 baud, ~20-100 byte packets)
- **Processing**: <1ms for decoding
- **Callback**: Depends on native code
- **Total**: ~10-50ms end-to-end

---

## Packet Protocol

### Serial Frame Structure

```
????????????????????????????????????????????????????????????????????
? 0xEF ? Length ? MsgType?PktID  ?Pipe  ? Payload ? CRC16LE ? 0xBE ?
?(Start)?+8      ?(Cmd)   ?(Seq)  ?(Sts) ?(N bytes)?(2 bytes)?(End) ?
????????????????????????????????????????????????????????????????????
   1      1         1       1      1     N bytes      2         1
```

- **CRC16**: Little-endian checksum of bytes between markers
- **Length**: Payload + 8 (overhead)
- **Pipe**: Upper 4 bits = channel, lower 4 bits = error code

### Message Types (Command Bytes)

| Hex | Name | Direction | Purpose |
|-----|------|-----------|---------|
| 0x10 | OmniSetMotionDataMessage | ? | Configure motion output |
| 0x11 | OmniMotionDataMessage | ? | Streaming motion data |
| 0x20 | OmniSetRawDataMessage | ? | Configure raw pod data |
| 0x21 | OmniRawDataMessage | ? | Streaming pod sensors |
| — | (Others) | — | Version, RSSI, etc. |

---

## Dependencies

### .NET Assemblies

- **System.IO.Ports**: SerialPort (Windows-specific)
- **System**: Base class library
- **System.Management** (optional): WMI port description queries

### External

- **OmniCommon.dll**: OmniCommon library (decompiled, in-repo)
- **openvr_driver.h** (C++ side): For SteamVR integration

---

## Building

### Requirements

- **.NET 10 SDK** or later
- **Visual Studio 2022** (C# and C++ workloads)
- **Windows 10/11**

### Build Commands

```bash
# Restore and build
dotnet build OmniBridge/OmniBridge.csproj -c Release

# Output: OmniBridge/bin/Release/net10.0/win-x64/OmniBridge.dll
```

### Integration with C++ Project

1. Copy `OmniBridge.dll` to C++ output directory
2. Copy `OmniCommon.dll` (dependency) to same directory
3. Link C++ project to load `OmniBridge.dll` dynamically (see P/Invoke example)

---

## Testing

### Unit Testing Checklist

- [ ] Port detection works (GetAvailablePorts)
- [ ] Port access validation (CanAccessPort)
- [ ] Message encoding/decoding (OmniPacketBuilder)
- [ ] CRC validation (correct & incorrect checksums)
- [ ] Event firing (MotionDataReceived)
- [ ] Thread safety (concurrent operations)
- [ ] Disconnection cleanup

### Integration Testing

```csharp
// Manual test with actual hardware
var handler = new OmniMotionDataHandler();
handler.MotionDataReceived += (s, data) =>
{
    Debug.WriteLine($"? Received angle: {data.RingAngle}");
};

if (handler.Connect())
{
    Task.Delay(10000).Wait();  // Run for 10 seconds
    handler.Disconnect();
}
```

---

## Troubleshooting

### Debugging Tips

1. **Enable verbose logging**:
   ```csharp
   handler.MotionDataReceived += (s, data) =>
       Console.WriteLine($"[{DateTime.Now:HH:mm:ss.fff}] Angle: {data.RingAngle}");
   ```

2. **Check CRC errors**:
   ```csharp
   handler.CrcErrorOccurred += (s, e) =>
       Console.WriteLine("? Packet checksum failed!");
   ```

3. **Monitor raw bytes** (RawHexDataReceived event)

4. **Test without hardware**:
   - Mock `OmniMotionDataHandler` for unit tests
   - Use recorded packet streams for integration tests

---

## Future Enhancements

- [ ] Async Connect/Disconnect (Task-based API)
- [ ] Reconnection with backoff strategy
- [ ] Multi-treadmill support
- [ ] .NET MAUI frontend for port selection
- [ ] Performance metrics (latency, packet loss)
- [ ] Simulation mode (synthetic angle data)

---

## License

This project is part of the Omni Pro Treadmill Driver initiative.  
See the main repository for licensing details.

---

## Support & Contributions

- **Issues**: Report on GitHub
- **Contributions**: Pull requests welcome
- **Documentation**: See OmniCommon/OmniTreadmill_Analyse.md for protocol details

