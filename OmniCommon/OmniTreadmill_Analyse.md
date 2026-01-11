# OmniTreadmill Library - Complete Analysis & Usage Guide

## 📋 Overview

The **OmniCommon** library is a .NET 10 library for communication with the Virtuix Omni Treadmill. It enables reading sensor data from the Pods (foot sensors), ring angle (Ring Angle), and gamepad data via a serial interface.

---

## 🏗️ Architecture

### Core Components

```
graph TB
    A[Your Application] --> B[OmniBaseMessage]
    B --> C[OmniPacketBuilder]
    C --> D[Serial Interface]
    D --> E[Omni Treadmill Hardware]
    
    B --> F[Message Types]
    F --> G[Motion Data Messages]
    F --> H[Raw Data Messages]
    F --> I[Control Messages]
    
    G --> J[OmniMotionDataMessage]
    G --> K[OmniMotionAndRawDataMessage]
    
    H --> L[OmniRawDataMessage]
    
    I --> M[OmniSetMotionDataMessage]
    I --> N[OmniSetRawDataMessage]
    I --> O[OmniVersionInfoMessage]
```

---

## 📦 Message Types (MessageType)

The library works with different message types for different purposes:

### 1. **Configuration Messages** (Send to Hardware)
| MessageType | Usage | Command |
|-------------|-------|---------|
| `OmniSetMotionDataMessage` | Configures which motion data is streamed | `SET_MOTION_DATA_MODE` |
| `OmniSetRawDataMessage` | Configures which raw data (pod sensors) is streamed | `SET_RAW_DATA_MODE` |
| `OmniVersionInfoMessage` | Queries version information | `GET_VERSION_INFO` |
| `OmniGetRSSIMessage` | Queries Bluetooth signal strength | `GET_GAZELL_RSSI` |
| `OmniSetSensitivityMessage` | Sets sensitivity | `SET_SENSITIVITY` |

### 2. **Data Reception Messages** (Received from Hardware)
| MessageType | Data Content | Command |
|-------------|--------------|---------|
| `OmniMotionDataMessage` | Motion data (angle, steps, gamepad) | `STREAM_MOTION_DATA` |
| `OmniRawDataMessage` | Raw pod sensor data (Quaternions, Accelerometer, Gyroscope) | `STREAM_RAW_DATA` |
| `OmniMotionAndRawDataMessage` | **Both combined** (Motion + Raw Data) | `STREAM_MOTION_AND_RAW_DATA` |

---

## 🎯 Data Types

### Motion Data (Movement Data)

**Available Fields:**
```
- Timestamp        // uint - Timestamp
- StepCount        // uint - Number of steps
- RingAngle        // float - Angle of the ring (user rotation)
- RingDelta        // byte - Change of angle
- GamePad_X        // byte - Gamepad X-axis
- GamePad_Y        // byte - Gamepad Y-axis
- GunButtonData    // byte - Gun button status
- StepTrigger      // byte - Step trigger
```

### Raw Data (Pod Sensor Data)

**Available per Pod:**
```
- Quaternions      // float[4] - Orientation (W, X, Y, Z)
- Accelerometer    // float[3] - Acceleration (X, Y, Z) in g
- Gyroscope        // float[3] - Rotation rate (X, Y, Z) in °/s
- FrameNumber      // ushort - Frame number
```

**Important:** The Omni has **2 Pods** (Pod1 = left foot, Pod2 = right foot)

---

## 🔧 Usage - Step by Step

### Step 1: Create and encode message

```
using OmniCommon;
using OmniCommon.Messages;

// Example 1: Configure motion data (enable all)
MotionDataSelection motionSelection = MotionDataSelection.AllOn();
OmniSetMotionDataMessage motionConfigMsg = new OmniSetMotionDataMessage(motionSelection);
byte[] packetToSend = motionConfigMsg.Encode();

// Example 2: Only specific motion data
MotionDataSelection customMotion = new MotionDataSelection
{
    Timestamp = true,
    StepCount = true,
    RingAngle = true,  // ← Important for angle!
    RingDelta = true,
    GamePadData = false,
    GunButtonData = false,
    StepTrigger = false
};
OmniSetMotionDataMessage customMsg = new OmniSetMotionDataMessage(customMotion);
byte[] customPacket = customMsg.Encode();
```

### Step 2: Configure raw data (pod sensors)

```
// Example 1: Enable all pod data (2 Pods)
RawDataSelection rawSelection = RawDataSelection.AllOn(2);
OmniSetRawDataMessage rawConfigMsg = new OmniSetRawDataMessage(rawSelection);
byte[] rawPacket = rawConfigMsg.Encode();

// Example 2: Only quaternions and accelerometer
RawDataSelection customRaw = new RawDataSelection
{
    Count = 2,  // 2 Pods
    Timestamp = true,
    Pods = new List<RawPodDataMode>
    {
        // Pod 1 (left foot)
        new RawPodDataMode
        {
            Quaternions = true,
            Accelerometer = true,
            Gyroscope = false,
            FrameNumber = false
        },
        // Pod 2 (right foot)
        new RawPodDataMode
        {
            Quaternions = true,
            Accelerometer = true,
            Gyroscope = false,
            FrameNumber = false
        }
    }
};
OmniSetRawDataMessage customRawMsg = new OmniSetRawDataMessage(customRaw);
byte[] customRawPacket = customRawMsg.Encode();
```

### Step 3: Send packet (via SerialPort)

```
using System.IO.Ports;

SerialPort omniPort = new SerialPort("COM3", 115200); // Adjust!
omniPort.Open();

// Send configuration packet
omniPort.Write(packetToSend, 0, packetToSend.Length);

// Optional: Wait briefly
System.Threading.Thread.Sleep(100);
```

### Step 4: Receive and decode data

```
// Buffer for incoming data
byte[] buffer = new byte[256];
int bytesRead = 0;

// Receive data
if (omniPort.BytesToRead > 0)
{
    bytesRead = omniPort.Read(buffer, 0, buffer.Length);
    
    // Decode the packet
    OmniBaseMessage receivedMsg = OmniPacketBuilder.decodePacket(buffer, bytesRead);
    
    if (receivedMsg != null)
    {
        // Check message type and process
        switch (receivedMsg.MsgType)
        {
            case MessageType.OmniMotionDataMessage:
                ProcessMotionData(receivedMsg);
                break;
                
            case MessageType.OmniRawDataMessage:
                ProcessRawData(receivedMsg);
                break;
                
            case MessageType.OmniMotionAndRawDataMessage:
                ProcessCombinedData(receivedMsg);
                break;
        }
    }
}
```

### Step 5: Extract data

#### A) Extract motion data

```
void ProcessMotionData(OmniBaseMessage baseMsg)
{
    OmniMotionDataMessage motionMsg = new OmniMotionDataMessage(baseMsg);
    OmniMotionData data = motionMsg.GetMotionData();
    
    if (data.EnableRingAngle)
    {
        float angle = data.RingAngle; // Angle in degrees (0-360)
        Console.WriteLine($"Ring Angle: {angle}°");
    }
    
    if (data.EnableStepCount)
    {
        uint steps = data.StepCount;
        Console.WriteLine($"Steps: {steps}");
    }
    
    if (data.EnableGamePadData)
    {
        byte x = data.GamePad_X;
        byte y = data.GamePad_Y;
        Console.WriteLine($"Gamepad: X={x}, Y={y}");
    }
}
```

#### B) Extract raw data (pod sensors)

```
void ProcessRawData(OmniBaseMessage baseMsg)
{
    OmniRawDataMessage rawMsg = new OmniRawDataMessage(baseMsg);
    OmniRawData data = rawMsg.GetRawData();
    
    // Access pod data
    for (int i = 0; i < data.PodData.Count; i++)
    {
        PodRawData pod = data.PodData[i];
        
        Console.WriteLine($"=== Pod {i + 1} ===");
        
        if (pod.EnableQuaternions && pod.Quaternions != null)
        {
            Console.WriteLine($"Quaternions: W={pod.Quaternions[0]}, " +
                            $"X={pod.Quaternions[1]}, " +
                            $"Y={pod.Quaternions[2]}, " +
                            $"Z={pod.Quaternions[3]}");
        }
        
        if (pod.EnableAccelerometer && pod.Accelerometer != null)
        {
            Console.WriteLine($"Accelerometer: X={pod.Accelerometer[0]}g, " +
                            $"Y={pod.Accelerometer[1]}g, " +
                            $"Z={pod.Accelerometer[2]}g");
        }
        
        if (pod.EnableGyroscope && pod.Gyroscope != null)
        {
            Console.WriteLine($"Gyroscope: X={pod.Gyroscope[0]}°/s, " +
                            $"Y={pod.Gyroscope[1]}°/s, " +
                            $"Z={pod.Gyroscope[2]}°/s");
        }
    }
}
```

#### C) Extract combined data (Motion + Raw)

```
void ProcessCombinedData(OmniBaseMessage baseMsg)
{
    OmniMotionAndRawDataMessage combinedMsg = new OmniMotionAndRawDataMessage(baseMsg);
    OmniMotionAndRawData data = combinedMsg.GetMotionAndRawData();
    
    // Motion data
    Console.WriteLine($"Timestamp: {data.Timestamp}");
    Console.WriteLine($"Steps: {data.StepCount}");
    Console.WriteLine($"Ring Angle: {data.RingAngle}°");
    
    // Pod 1 data
    if (data.Pod1Quaternions != null)
    {
        Console.WriteLine($"Pod1 Quaternions: {string.Join(", ", data.Pod1Quaternions)}");
    }
    
    if (data.Pod1Accelerometer != null)
    {
        Console.WriteLine($"Pod1 Accel: {string.Join(", ", data.Pod1Accelerometer)}");
    }
    
    // Pod 2 data
    if (data.Pod2Quaternions != null)
    {
        Console.WriteLine($"Pod2 Quaternions: {string.Join(", ", data.Pod2Quaternions)}");
    }
}
```

---

## 🎮 Important Commands

### Control Commands

```
// Query version info
OmniVersionInfoMessage versionMsg = new OmniVersionInfoMessage();
byte[] versionPacket = versionMsg.Encode();
omniPort.Write(versionPacket, 0, versionPacket.Length);

// Query RSSI (Bluetooth signal strength)
OmniGetRSSIMessage rssiMsg = new OmniGetRSSIMessage();
byte[] rssiPacket = rssiMsg.Encode();
omniPort.Write(rssiPacket, 0, rssiPacket.Length);

// Reset treadmill
OmniResetTivaMessage resetMsg = new OmniResetTivaMessage();
byte[] resetPacket = resetMsg.Encode();
omniPort.Write(resetPacket, 0, resetPacket.Length);
```

---

## 🔍 Packet Structure

```
┌─────────────────────────────────────────────────┐
│ Byte 0: 0xEF (Start marker)                    │
├─────────────────────────────────────────────────┤
│ Byte 1: Packet length (Payload + 8)            │
├─────────────────────────────────────────────────┤
│ Byte 2: Command/MessageType                     │
├─────────────────────────────────────────────────┤
│ Byte 3: Packet ID                               │
├─────────────────────────────────────────────────┤
│ Byte 4: Pipe/Status byte                        │
│   - Bits 4-7: Pipe number                       │
│   - Bits 1-3: Error code                        │
│   - Bit 0: IsResponse flag                      │
├─────────────────────────────────────────────────┤
│ Byte 5-N: Payload data                          │
├─────────────────────────────────────────────────┤
│ Byte N+1, N+2: CRC16 (Little Endian)           │
├─────────────────────────────────────────────────┤
│ Byte N+3: 0xBE (End marker)                    │
└─────────────────────────────────────────────────┘
```

---

## 💡 Best Practices

### 1. **Data Mode Switching**

```
// To switch between modes, send new configuration messages

// Enable motion data
MotionDataSelection motionSel = MotionDataSelection.AllOn();
SendMessage(new OmniSetMotionDataMessage(motionSel));

// As needed: Enable raw data (can run in parallel)
RawDataSelection rawSel = RawDataSelection.AllOn(2);
SendMessage(new OmniSetRawDataMessage(rawSel));

// Or: Disable all
SendMessage(new OmniSetMotionDataMessage(MotionDataSelection.AllOff()));
SendMessage(new OmniSetRawDataMessage(RawDataSelection.AllOff()));
```

### 2. **Efficient Data Retrieval**

```
// Enable only required data for better performance
MotionDataSelection efficientMotion = new MotionDataSelection
{
    Timestamp = true,
    StepCount = false,  // Not needed
    RingAngle = true,   // Needed for direction
    RingDelta = false,
    GamePadData = false,
    GunButtonData = false,
    StepTrigger = true  // Needed for step detection
};
```

### 3. **Error Handling**

```
OmniBaseMessage msg = OmniPacketBuilder.decodePacket(buffer, bytesRead);

if (msg == null)
{
    Console.WriteLine("CRC error or invalid packet!");
    return;
}

if (msg.ErrorCode != 0)
{
    Console.WriteLine($"Hardware error: {msg.ErrorCode}");
}

if (msg.IsResponse)
{
    Console.WriteLine("This is a response to a previous command");
}
```

### 4. **Continuous Reading Loop**

```
bool isRunning = true;
byte[] buffer = new byte[256];

while (isRunning)
{
    try
    {
        if (omniPort.BytesToRead > 0)
        {
            int bytesRead = omniPort.Read(buffer, 0, buffer.Length);
            OmniBaseMessage msg = OmniPacketBuilder.decodePacket(buffer, bytesRead);
            
            if (msg != null)
            {
                ProcessMessage(msg);
            }
        }
        
        System.Threading.Thread.Sleep(10); // Reduce CPU usage
    }
    catch (Exception ex)
    {
        Console.WriteLine($"Error: {ex.Message}");
    }
}
```

---

## 📊 Data Conversion

### Ring Angle
- **Value range:** 0.0 to 360.0 degrees
- **Type:** `float`
- **Meaning:** Absolute direction of the user

### Quaternions (Orientation)
- **Array:** `float[4]` → `[W, X, Y, Z]`
- **Normalized:** Length should be ≈ 1.0
- **Conversion to Euler angles possible**

### Accelerometer
- **Array:** `float[3]` → `[X, Y, Z]`
- **Unit:** g (gravitational acceleration)
- **Scaling:** Value / 4096.0
- **Range:** Typically ±2g to ±8g

### Gyroscope
- **Array:** `float[3]` → `[X, Y, Z]`
- **Unit:** Degrees per second (°/s)
- **Scaling:** Value / 1024.0
- **Range:** Typically ±250°/s to ±2000°/s

---

## 🚀 Summary

### Basic Workflow

```
sequenceDiagram
    participant App as Your App
    participant Lib as OmniCommon
    participant HW as Omni Hardware
    
    App->>Lib: Create OmniSetMotionDataMessage
    Lib->>App: Encode() → byte[]
    App->>HW: SerialPort.Write(bytes)
    
    Note over HW: Hardware processes configuration
    
    HW->>App: Sends motion data stream
    App->>Lib: decodePacket(buffer)
    Lib->>App: OmniBaseMessage
    App->>Lib: GetMotionData()
    Lib->>App: OmniMotionData with all values
    
    App->>App: Process RingAngle, steps, etc.
```

### Switch Overview

| What You Want | Which Message | Important Properties |
|---------------|---------------|----------------------|
| Get angle data | `OmniSetMotionDataMessage` | `RingAngle = true` |
| Get pod quaternions | `OmniSetRawDataMessage` | `Pods[i].Quaternions = true` |
| Get acceleration data | `OmniSetRawDataMessage` | `Pods[i].Accelerometer = true` |
| Get gyroscope data | `OmniSetRawDataMessage` | `Pods[i].Gyroscope = true` |
| Get step counter | `OmniSetMotionDataMessage` | `StepCount = true` |
| Get gamepad data | `OmniSetMotionDataMessage` | `GamePadData = true` |
| Get everything at once | `OmniMotionAndRawDataMessage` | Combines both modes |

---

## ⚠️ Important Notes

1. **Thread-Safety:** The library is NOT thread-safe. Use locks for multi-threading!

2. **COM Port:** The correct COM port must be determined (e.g., "COM3", "COM4", etc.)

3. **Baud Rate:** Default **115200** for Omni Treadmill

4. **Packet Size:** Keep maximum payload size in mind (Type `byte` for Length)

5. **CRC Check:** Always check if `decodePacket()` returns `null`

6. **Timing:** Wait briefly after configuration messages (100ms) before expecting data

---

## 📚 Message Classes Reference

### Configuration Classes

#### MotionDataSelection
```
public class MotionDataSelection
{
    public bool Timestamp;
    public bool StepCount;
    public bool RingAngle;
    public bool RingDelta;
    public bool GamePadData;
    public bool GunButtonData;
    public bool StepTrigger;
    
    // Factory methods
    public static MotionDataSelection AllOn();
    public static MotionDataSelection AllOff();
    public static MotionDataSelection StreamingWindowData();
}
```

#### RawDataSelection
```
public class RawDataSelection
{
    public int Count;  // Number of pods (usually 2)
    public bool Timestamp;
    public List<RawPodDataMode> Pods;
    
    // Factory methods
    public static RawDataSelection AllOn(int count);
    public static RawDataSelection AllOff(int count);
    public static RawDataSelection AllOff();
}
```

#### RawPodDataMode
```
public class RawPodDataMode
{
    public bool Quaternions;
    public bool Accelerometer;
    public bool Gyroscope;
    public bool FrameNumber;
    
    public byte GetByte();  // Converts to byte representation
}
```

### Data Classes

#### OmniMotionData
```
public class OmniMotionData
{
    public bool EnableTimestamp;
    public bool EnableStepCount;
    public bool EnableRingAngle;
    public bool EnableRingDelta;
    public bool EnableGamePadData;
    public bool EnableGunButtonData;
    public bool EnableStepTrigger;
    
    public uint Timestamp;
    public uint StepCount;
    public float RingAngle;
    public byte RingDelta;
    public byte GamePad_X;
    public byte GamePad_Y;
    public byte GunButtonData;
    public byte StepTrigger;
}
```

#### OmniRawData
```
public class OmniRawData
{
    public int Count;
    public bool EnableTimestamp;
    public uint Timestamp;
    public List<RawPodDataMode> Pods;
    public List<PodRawData> PodData;
}
```

#### PodRawData
```
public class PodRawData
{
    public bool EnableQuaternions;
    public bool EnableAccelerometer;
    public bool EnableGyroscope;
    
    public float[] Quaternions;     // [4] - W, X, Y, Z
    public float[] Accelerometer;   // [3] - X, Y, Z in g
    public float[] Gyroscope;       // [3] - X, Y, Z in °/s
    public ushort FrameNumber;
}
```

#### OmniMotionAndRawData
```
public class OmniMotionAndRawData
{
    // Motion data
    public uint Timestamp;
    public uint StepCount;
    public float RingAngle;
    public byte RingDelta;
    public byte GamePad_X;
    public byte GamePad_Y;
    public byte StepTrigger;
    
    // Pod 1 raw data
    public float[] Pod1Quaternions;
    public float[] Pod1Accelerometer;
    public float[] Pod1Gyroscope;
    
    // Pod 2 raw data
    public float[] Pod2Quaternions;
    public float[] Pod2Accelerometer;
    public float[] Pod2Gyroscope;
}
```

---

## 🔧 Advanced Examples

### Example 1: Complete Initialization

```
using System;
using System.IO.Ports;
using System.Threading;
using OmniCommon;
using OmniCommon.Messages;

public class OmniTreadmillController
{
    private SerialPort port;
    private Thread readerThread;
    private bool isRunning;
    
    public event EventHandler<OmniMotionData> MotionDataReceived;
    public event EventHandler<OmniRawData> RawDataReceived;
    
    public bool Connect(string comPort)
    {
        try
        {
            port = new SerialPort(comPort, 115200);
            port.Open();
            
            // Configure motion data
            MotionDataSelection motionSel = MotionDataSelection.AllOn();
            SendMessage(new OmniSetMotionDataMessage(motionSel));
            
            Thread.Sleep(100);
            
            // Configure raw data for 2 pods
            RawDataSelection rawSel = RawDataSelection.AllOn(2);
            SendMessage(new OmniSetRawDataMessage(rawSel));
            
            // Start reader thread
            isRunning = true;
            readerThread = new Thread(ReadLoop);
            readerThread.Start();
            
            return true;
        }
        catch (Exception ex)
        {
            Console.WriteLine($"Connection error: {ex.Message}");
            return false;
        }
    }
    
    public void Disconnect()
    {
        isRunning = false;
        
        if (readerThread != null && readerThread.IsAlive)
        {
            readerThread.Join(1000);
        }
        
        if (port != null && port.IsOpen)
        {
            port.Close();
        }
    }
    
    private void SendMessage(OmniBaseMessage message)
    {
        byte[] packet = message.Encode();
        port.Write(packet, 0, packet.Length);
    }
    
    private void ReadLoop()
    {
        byte[] buffer = new byte[256];
        
        while (isRunning)
        {
            try
            {
                if (port.BytesToRead > 0)
                {
                    int bytesRead = port.Read(buffer, 0, buffer.Length);
                    OmniBaseMessage msg = OmniPacketBuilder.decodePacket(buffer, bytesRead);
                    
                    if (msg != null)
                    {
                        ProcessMessage(msg);
                    }
                }
                
                Thread.Sleep(10);
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Read error: {ex.Message}");
            }
        }
    }
    
    private void ProcessMessage(OmniBaseMessage msg)
    {
        switch (msg.MsgType)
        {
            case MessageType.OmniMotionDataMessage:
                OmniMotionDataMessage motionMsg = new OmniMotionDataMessage(msg);
                OmniMotionData motionData = motionMsg.GetMotionData();
                MotionDataReceived?.Invoke(this, motionData);
                break;
                
            case MessageType.OmniRawDataMessage:
                OmniRawDataMessage rawMsg = new OmniRawDataMessage(msg);
                OmniRawData rawData = rawMsg.GetRawData();
                RawDataReceived?.Invoke(this, rawData);
                break;
                
            case MessageType.OmniMotionAndRawDataMessage:
                OmniMotionAndRawDataMessage combinedMsg = new OmniMotionAndRawDataMessage(msg);
                OmniMotionAndRawData combinedData = combinedMsg.GetMotionAndRawData();
                // Process combined data
                break;
        }
    }
}
```

### Example 2: Using the Controller

```
class Program
{
    static void Main(string[] args)
    {
        OmniTreadmillController controller = new OmniTreadmillController();
        
        // Register event handlers
        controller.MotionDataReceived += OnMotionDataReceived;
        controller.RawDataReceived += OnRawDataReceived;
        
        // Connect
        if (controller.Connect("COM3"))
        {
            Console.WriteLine("Connected to Omni Treadmill");
            Console.WriteLine("Press any key to exit...");
            Console.ReadKey();
            
            controller.Disconnect();
        }
        else
        {
            Console.WriteLine("Connection failed!");
        }
    }
    
    static void OnMotionDataReceived(object sender, OmniMotionData data)
    {
        Console.WriteLine($"Ring Angle: {data.RingAngle:F2}°");
        Console.WriteLine($"Steps: {data.StepCount}");
        Console.WriteLine($"Timestamp: {data.Timestamp}");
        Console.WriteLine();
    }
    
    static void OnRawDataReceived(object sender, OmniRawData data)
    {
        for (int i = 0; i < data.PodData.Count; i++)
        {
            PodRawData pod = data.PodData[i];
            Console.WriteLine($"Pod {i + 1}:");
            
            if (pod.Quaternions != null)
            {
                Console.WriteLine($"  Quat: [{pod.Quaternions[0]:F3}, {pod.Quaternions[1]:F3}, " +
                                $"{pod.Quaternions[2]:F3}, {pod.Quaternions[3]:F3}]");
            }
            
            if (pod.Accelerometer != null)
            {
                Console.WriteLine($"  Accel: [{pod.Accelerometer[0]:F3}g, " +
                                $"{pod.Accelerometer[1]:F3}g, {pod.Accelerometer[2]:F3}g]");
            }
        }
        Console.WriteLine();
    }
}
```

### Example 3: Quaternion to Euler Angle Conversion

```
public class QuaternionHelper
{
    public static void QuaternionToEuler(float[] q, out float roll, out float pitch, out float yaw)
    {
        // q = [W, X, Y, Z]
        float w = q[0];
        float x = q[1];
        float y = q[2];
        float z = q[3];
        
        // Roll (X-axis)
        float sinr_cosp = 2.0f * (w * x + y * z);
        float cosr_cosp = 1.0f - 2.0f * (x * x + y * y);
        roll = (float)Math.Atan2(sinr_cosp, cosr_cosp);
        
        // Pitch (Y-axis)
        float sinp = 2.0f * (w * y - z * x);
        if (Math.Abs(sinp) >= 1)
            pitch = (float)Math.CopySign(Math.PI / 2, sinp); // ±90°
        else
            pitch = (float)Math.Asin(sinp);
        
        // Yaw (Z-axis)
        float siny_cosp = 2.0f * (w * z + x * y);
        float cosy_cosp = 1.0f - 2.0f * (y * y + z * z);
        yaw = (float)Math.Atan2(siny_cosp, cosy_cosp);
        
        // Convert to degrees
        roll = roll * 180.0f / (float)Math.PI;
        pitch = pitch * 180.0f / (float)Math.PI;
        yaw = yaw * 180.0f / (float)Math.PI;
    }
}

// Usage
float[] quaternions = podData.Quaternions;
float roll, pitch, yaw;
QuaternionHelper.QuaternionToEuler(quaternions, out roll, out pitch, out yaw);
Console.WriteLine($"Roll: {roll:F2}°, Pitch: {pitch:F2}°, Yaw: {yaw:F2}°");
```

---

## 🐛 Troubleshooting

### Problem: No Data Received

**Solution:**
```
// 1. Check the connection
if (!port.IsOpen)
{
    Console.WriteLine("Port is not open!");
}

// 2. Check if configuration messages were sent
MotionDataSelection selection = MotionDataSelection.AllOn();
OmniSetMotionDataMessage msg = new OmniSetMotionDataMessage(selection);
byte[] packet = msg.Encode();
port.Write(packet, 0, packet.Length);
Thread.Sleep(200); // Wait longer

// 3. Check the baud rate
// Omni uses 115200
```

### Problem: CRC Error

**Solution:**
```
// Make sure you read the complete message
byte[] buffer = new byte[256];
int bytesRead = 0;

// Wait until enough data is available
while (port.BytesToRead < 8) // Minimum packet size
{
    Thread.Sleep(10);
}

bytesRead = port.Read(buffer, 0, Math.Min(port.BytesToRead, buffer.Length));

OmniBaseMessage msg = OmniPacketBuilder.decodePacket(buffer, bytesRead);
if (msg == null)
{
    Console.WriteLine("CRC error or incomplete data");
    // Flush buffer
    port.DiscardInBuffer();
}
```

### Problem: Threading Issues

**Solution:**
```
// Use locks for thread-safety
private object lockObject = new object();

private void SendMessage(OmniBaseMessage message)
{
    lock (lockObject)
    {
        byte[] packet = message.Encode();
        port.Write(packet, 0, packet.Length);
    }
}

private void ReadLoop()
{
    while (isRunning)
    {
        lock (lockObject)
        {
            if (port.BytesToRead > 0)
            {
                // Read data...
            }
        }
        Thread.Sleep(10);
    }
}
```

---

## 📖 Glossary

| Term | Meaning |
|------|---------|
| **Pod** | Foot sensor of Omni Treadmill (left/right) |
| **Ring Angle** | Rotation angle of user on treadmill (0-360°) |
| **Quaternion** | 4D representation of orientation (W, X, Y, Z) |
| **Accelerometer** | Acceleration sensor (measures linear acceleration) |
| **Gyroscope** | Rotation rate sensor (measures angular velocity) |
| **CRC16** | Cyclic Redundancy Check - error detection checksum |
| **Payload** | Useful data within a packet |
| **Pipe** | Communication channel (Bits 4-7 of status byte) |
| **RSSI** | Received Signal Strength Indicator - signal strength |

---