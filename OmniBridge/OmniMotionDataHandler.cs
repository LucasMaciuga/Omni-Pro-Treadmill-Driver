using System.IO.Ports;
using OmniCommon;
using OmniCommon.Messages;

/// <summary>
/// Manages the configuration and reception of OmniMotionDataMessage
/// </summary>
public class OmniMotionDataHandler : IDisposable
{
    private readonly SerialPort _port;
    private readonly Thread _readerThread;
    private bool _isRunning;
    private readonly object _lockObject = new();
    private readonly int _configurationDelayMs;
    private OmniMode _currentMode = OmniMode.DecoupledForwardBackStrafe;
    private int _crcErrorCount = 0;

    /// <summary>
    /// Event is raised when motion data is received
    /// </summary>
    public event EventHandler<OmniMotionData>? MotionDataReceived;

    /// <summary>
    /// Event is raised when raw hex data is received
    /// </summary>
    public event EventHandler<RawHexDataEventArgs>? RawHexDataReceived;

    /// <summary>
    /// Event is raised when a connection error occurs
    /// </summary>
    public event EventHandler<string>? ConnectionError;

    /// <summary>
    /// Event is raised when the OmniMode is changed
    /// </summary>
    public event EventHandler<OmniMode>? ModeChanged;

    /// <summary>
    /// Event is raised when a CRC error occurs
    /// </summary>
    public event EventHandler? CrcErrorOccurred;

    public bool IsConnected => _port?.IsOpen ?? false;
    public string PortName { get; }
    public int BaudRate { get; }
    public OmniMode CurrentMode => _currentMode;
    
    /// <summary>
    /// Returns the number of CRC errors that have occurred
    /// </summary>
    public int CrcErrorCount => _crcErrorCount;

    public OmniMotionDataHandler(string comPort = "COM3", int baudRate = 115200, int configurationDelayMs = 100, OmniMode initialMode = OmniMode.ForwardBackStrafe)
    {
        PortName = comPort;
        BaudRate = baudRate;
        _configurationDelayMs = configurationDelayMs;
        _currentMode = initialMode;

        _port = new SerialPort(comPort, baudRate)
        {
            ReadTimeout = 500,
            WriteTimeout = 500
        };

        _readerThread = new Thread(ReadLoop)
        {
            IsBackground = true,
            Name = "OmniMotionDataReader"
        };
    }

    /// <summary>
    /// Connects to the Omni Treadmill and configures Motion Data streaming
    /// </summary>
    /// <param name="selection">The Motion Data Selection - null for AllOn()</param>
    /// <param name="omniMode">The OmniMode - null for standard</param>
    public bool Connect(MotionDataSelection? selection = null, OmniMode? omniMode = null)
    {
        try
        {
            if (_port.IsOpen)
            {
                Console.WriteLine("Port is already open.");
                return true;
            }

            // Check if port exists
            if (!ComPortHelper.IsPortAvailable(PortName))
            {
                var error = $"COM port '{PortName}' does not exist on this system!";
                Console.WriteLine(error);
                ConnectionError?.Invoke(this, error);
                return false;
            }

            // Check if port is accessible
            if (!ComPortHelper.CanAccessPort(PortName, BaudRate))
            {
                var error = $"COM port '{PortName}' is not accessible (possibly used by another program)!";
                Console.WriteLine(error);
                ConnectionError?.Invoke(this, error);
                return false;
            }

            _port.Open();
            Console.WriteLine($"Connected to {_port.PortName} @ {_port.BaudRate} baud");

            // Set OmniMode (use settings mode or keep current)
            var modeToSet = omniMode ?? _currentMode;
            SetMode(modeToSet);

            Thread.Sleep(_configurationDelayMs); // Wait for hardware response

            // Configure Motion Data (default: enable all)
            var motionSelection = selection ?? MotionDataSelection.AllOn();
            ConfigureMotionData(motionSelection);

            Thread.Sleep(_configurationDelayMs); // Wait for hardware response

            // Start reader thread
            _isRunning = true;
            _readerThread.Start();

            Console.WriteLine("Motion Data streaming enabled.");
            return true;
        }
        catch (UnauthorizedAccessException ex)
        {
            var error = $"Access denied to {PortName}: {ex.Message}";
            Console.WriteLine(error);
            Console.WriteLine("Possible causes:");
            Console.WriteLine("  - Port is used by another program");
            Console.WriteLine("  - Insufficient permissions");
            Console.WriteLine("  - USB device not properly connected");
            ConnectionError?.Invoke(this, error);
            return false;
        }
        catch (IOException ex)
        {
            var error = $"I/O error when opening {PortName}: {ex.Message}";
            Console.WriteLine(error);
            ConnectionError?.Invoke(this, error);
            return false;
        }
        catch (Exception ex)
        {
            var error = $"Connection error: {ex.Message}";
            Console.WriteLine(error);
            ConnectionError?.Invoke(this, error);
            return false;
        }
    }

    /// <summary>
    /// Sets the OmniMode (movement mode)
    /// </summary>
    public void SetMode(OmniMode mode)
    {
        if (!_port.IsOpen)
        {
            throw new InvalidOperationException("Port is not open!");
        }

        var previousMode = _currentMode;
        _currentMode = mode;

        var message = new OmniChangeGamepadModeMessage((byte)mode);
        SendMessage(message);

        Console.WriteLine($"OmniMode set: {mode} ({(int)mode})");

        ModeChanged?.Invoke(this, mode);
    }

    /// <summary>
    /// Configures which motion data should be streamed
    /// </summary>
    public void ConfigureMotionData(MotionDataSelection selection)
    {
        if (!_port.IsOpen)
        {
            throw new InvalidOperationException("Port is not open!");
        }

        var message = new OmniSetMotionDataMessage(selection);
        SendMessage(message);

        Console.WriteLine("Motion Data configuration sent:");
        Console.WriteLine($"  - Timestamp: {selection.Timestamp}");
        Console.WriteLine($"  - StepCount: {selection.StepCount}");
        Console.WriteLine($"  - RingAngle: {selection.RingAngle}");
        Console.WriteLine($"  - RingDelta: {selection.RingDelta}");
        Console.WriteLine($"  - GamePadData: {selection.GamePadData}");
        Console.WriteLine($"  - GunButtonData: {selection.GunButtonData}");
        Console.WriteLine($"  - StepTrigger: {selection.StepTrigger}");
    }

    /// <summary>
    /// Creates a custom Motion Data Selection
    /// </summary>
    public static MotionDataSelection CreateCustomSelection(
        bool timestamp = true,
        bool stepCount = true,
        bool ringAngle = true,
        bool ringDelta = false,
        bool gamePadData = false,
        bool gunButtonData = false,
        bool stepTrigger = true)
    {
        return new MotionDataSelection
        {
            Timestamp = timestamp,
            StepCount = stepCount,
            RingAngle = ringAngle,
            RingDelta = ringDelta,
            GamePadData = gamePadData,
            GunButtonData = gunButtonData,
            StepTrigger = stepTrigger
        };
    }

    /// <summary>
    /// Disconnects from the treadmill
    /// </summary>
    public void Disconnect()
    {
        _isRunning = false;

        if (_readerThread.IsAlive)
        {
            if (!_readerThread.Join(1000))
            {
                Console.WriteLine("Warning: Reader thread could not be terminated cleanly.");
            }
        }

        if (_port.IsOpen)
        {
            // Disable motion data streaming before closing
            try
            {
                var offSelection = MotionDataSelection.AllOff();
                var message = new OmniSetMotionDataMessage(offSelection);
                SendMessage(message);
                Thread.Sleep(100);
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error disabling streaming: {ex.Message}");
            }

            _port.Close();
            Console.WriteLine("Disconnected.");
        }
    }

    private void SendMessage(OmniBaseMessage message)
    {
        lock (_lockObject)
        {
            byte[] packet = message.Encode();
            _port.Write(packet, 0, packet.Length);
        }
    }

    private void ReadLoop()
    {
        byte[] buffer = new byte[256];

        while (_isRunning)
        {
            try
            {
                if (_port.BytesToRead > 0)
                {
                    int bytesRead;
                    
                    lock (_lockObject)
                    {
                        bytesRead = _port.Read(buffer, 0, Math.Min(_port.BytesToRead, buffer.Length));
                    }

                    // Fire Raw Hex Event
                    OnRawHexDataReceived(buffer, bytesRead);

                    // Decode packet
                    OmniBaseMessage? msg = OmniPacketBuilder.decodePacket(buffer, bytesRead);

                    if (msg != null)
                    {
                        ProcessMessage(msg);
                    }
                    else
                    {
                        // CRC error: count instead of output
                        _crcErrorCount++;
                        CrcErrorOccurred?.Invoke(this, EventArgs.Empty);
                    }
                }

                Thread.Sleep(10); // Reduce CPU usage
            }
            catch (TimeoutException)
            {
                // Normal on ReadTimeout
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Read error: {ex.Message}");
                Thread.Sleep(100);
            }
        }
    }

    private void ProcessMessage(OmniBaseMessage msg)
    {
        if (msg.ErrorCode != 0)
        {
            Console.WriteLine($"Hardware error code: {msg.ErrorCode}");
        }

        if (msg.MsgType == MessageType.OmniMotionDataMessage)
        {
            var motionMsg = new OmniMotionDataMessage(msg);
            var motionData = motionMsg.GetMotionData();
            OnMotionDataReceived(motionData);
        }
        else
        {
            Console.WriteLine($"Unexpected message type received: {msg.MsgType}");
        }
    }

    private void OnMotionDataReceived(OmniMotionData data)
    {
        MotionDataReceived?.Invoke(this, data);
    }

    private void OnRawHexDataReceived(byte[] buffer, int length)
    {
        RawHexDataReceived?.Invoke(this, new RawHexDataEventArgs(buffer, length));
    }

    public void Dispose()
    {
        Disconnect();
        _port?.Dispose();
    }
}

/// <summary>
/// Event arguments for raw hex data
/// </summary>
public class RawHexDataEventArgs : EventArgs
{
    public byte[] Data { get; }
    public int Length { get; }
    public DateTime Timestamp { get; }

    public RawHexDataEventArgs(byte[] data, int length)
    {
        Data = new byte[length];
        Array.Copy(data, Data, length);
        Length = length;
        Timestamp = DateTime.Now;
    }

    /// <summary>
    /// Converts the data to a hex string
    /// </summary>
    public string ToHexString()
    {
        return BitConverter.ToString(Data, 0, Length).Replace("-", " ");
    }

    /// <summary>
    /// Converts the data to a formatted hex string with offsets
    /// </summary>
    public string ToFormattedHexString()
    {
        var sb = new System.Text.StringBuilder();
        for (int i = 0; i < Length; i++)
        {
            if (i % 16 == 0)
            {
                if (i > 0) sb.AppendLine();
                sb.Append($"{i:X4}:  ");
            }
            sb.Append($"{Data[i]:X2} ");
        }
        return sb.ToString();
    }
}

/// <summary>
/// Extension methods for OmniMotionData for better data output
/// </summary>
public static class OmniMotionDataExtensions
{
    /// <summary>
    /// Creates a formatted string representation of the motion data
    /// </summary>
    public static string ToFormattedString(this OmniMotionData data)
    {
        var lines = new List<string>();

        if (data.EnableTimestamp)
            lines.Add($"Timestamp: {data.Timestamp} ms");

        if (data.EnableStepCount)
            lines.Add($"Steps: {data.StepCount}");

        if (data.EnableRingAngle)
            lines.Add($"Ring Angle: {data.RingAngle:F2}°");

        if (data.EnableRingDelta)
            lines.Add($"Ring Delta: {data.RingDelta}");

        if (data.EnableGamePadData)
            lines.Add($"GamePad: X={data.GamePad_X}, Y={data.GamePad_Y}");

        if (data.EnableGunButtonData)
            lines.Add($"Gun Button: {data.GunButtonData}");

        if (data.EnableStepTrigger)
            lines.Add($"Step Trigger: {data.StepTrigger}");

        return string.Join(Environment.NewLine, lines);
    }

    /// <summary>
    /// Creates a compact single-line string representation
    /// </summary>
    public static string ToCompactString(this OmniMotionData data)
    {
        var parts = new List<string>();

        if (data.EnableTimestamp)
            parts.Add($"T:{data.Timestamp}");

        if (data.EnableStepCount)
            parts.Add($"S:{data.StepCount}");

        if (data.EnableRingAngle)
            parts.Add($"A:{data.RingAngle:F1}°");

        if (data.EnableGamePadData)
            parts.Add($"GP:{data.GamePad_X}/{data.GamePad_Y}");

        return string.Join(" ", parts);
    }
}