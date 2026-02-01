using System.Runtime.InteropServices;
using OmniCommon;
using OmniCommon.Messages;
using OmniBridge;

public class MinimalOmniReader
{
    private OmniMotionDataHandler? _handler;
    private nint _callbackPtr;
    private TreadmillSharedMemory? _sharedMemory;
    private bool _isMaster;
    private bool _isConsumer;
    private Thread? _consumerThread;
    private volatile bool _running;
    private float _speedMultiplier = 1.5f;
    private float _deadzone = 0.1f;
    private float _smoothing = 0.3f;
    private float _lastX, _lastY;
    private int _packetCount = 0;
    
    // Connection parameters for failover
    private string _comPort = "COM3";
    private int _baudRate = 115200;
    private int _omniMode = 0;
    private const int StaleDataThresholdMs = 1000;

    [UnmanagedCallersOnly(EntryPoint = "OmniReader_Create")]
    public static nint Create()
    {
        Logger.Debug("Create() called");
        var reader = new MinimalOmniReader();
        var handle = GCHandle.Alloc(reader);
        return GCHandle.ToIntPtr(handle);
    }

    [UnmanagedCallersOnly(EntryPoint = "OmniReader_Initialize")]
    public static bool Initialize(nint handle, nint comPortPtr, int omniMode, int baudRate)
    {
        try
        {
            var reader = GetReader(handle);
            string comPort = Marshal.PtrToStringUTF8(comPortPtr) ?? "COM3";
            
            Logger.Info($"Initialize: COM={comPort}, Mode={omniMode}, Baud={baudRate}");
            
            // Store for potential failover
            reader._comPort = comPort;
            reader._baudRate = baudRate;
            reader._omniMode = omniMode;
            
            // Try shared memory consumer mode first (non-blocking, fail-safe)
            if (reader.TryConnectAsConsumer())
            {
                Logger.Info("Running as CONSUMER (reading from shared memory)");
                return true;
            }
            
            // No master available or shared memory failed - become master with direct COM port
            Logger.Debug("No master found, initializing as MASTER (direct COM port)");
            return reader.InitializeDirectMode(comPort, omniMode, baudRate);
        }
        catch (Exception ex)
        {
            Logger.Error("Initialize FAILED", ex);
            return false;
        }
    }

    /// <summary>
    /// Try to connect as shared memory consumer. Returns false if not possible.
    /// </summary>
    private bool TryConnectAsConsumer()
    {
        try
        {
            // Check if master exists
            if (!TreadmillSharedMemory.IsMasterRunning())
            {
                Logger.Debug("TryConnectAsConsumer: No master running");
                return false;
            }
            
            Logger.Debug("TryConnectAsConsumer: Master found, connecting...");
            _sharedMemory = new TreadmillSharedMemory();
            if (!_sharedMemory.InitializeAsConsumer())
            {
                Logger.Debug("TryConnectAsConsumer: InitializeAsConsumer failed");
                _sharedMemory.Dispose();
                _sharedMemory = null;
                return false;
            }
            
            _isConsumer = true;
            _isMaster = false;
            _running = true;
            
            // Start consumer thread
            _consumerThread = new Thread(ConsumerLoop)
            {
                IsBackground = true,
                Name = "OmniBridge_Consumer"
            };
            _consumerThread.Start();
            
            Logger.Info("Consumer mode active - reading from shared memory");
            return true;
        }
        catch (Exception ex)
        {
            Logger.Error("TryConnectAsConsumer failed", ex);
            // Any error - clean up and return false
            _sharedMemory?.Dispose();
            _sharedMemory = null;
            return false;
        }
    }

    /// <summary>
    /// Initialize direct COM port mode (master mode).
    /// </summary>
    private bool InitializeDirectMode(string comPort, int omniMode, int baudRate)
    {
        try
        {
            Logger.Debug($"InitializeDirectMode: Opening {comPort}...");
            _handler = new OmniMotionDataHandler(comPort, baudRate, 100, (OmniMode)omniMode);

            var selection = new MotionDataSelection
            {
                RingAngle = true,
                GamePadData = true,
                Timestamp = false,
                StepCount = false,
                RingDelta = false,
                GunButtonData = false,
                StepTrigger = false
            };

            if (!_handler.Connect(selection, (OmniMode)omniMode))
            {
                Logger.Error("InitializeDirectMode: Connect FAILED");
                return false;
            }

            Logger.Debug($"InitializeDirectMode: COM port connected. IsConnected={_handler.IsConnected}");
            
            _isMaster = true;
            _isConsumer = false;
            _running = true;
            
            // Try to initialize shared memory for other processes (optional, non-critical)
            TryInitializeSharedMemoryAsMaster();

            _handler.MotionDataReceived += OnMotionDataReceived;
            
            Logger.Info($"Master mode active - connected to {comPort}");
            return true;
        }
        catch (Exception ex)
        {
            Logger.Error("InitializeDirectMode failed", ex);
            return false;
        }
    }

    /// <summary>
    /// Try to set up shared memory as master. Failure is non-critical.
    /// </summary>
    private void TryInitializeSharedMemoryAsMaster()
    {
        try
        {
            Logger.Debug("TryInitializeSharedMemoryAsMaster: Creating shared memory...");
            _sharedMemory = new TreadmillSharedMemory();
            if (!_sharedMemory.InitializeAsMaster())
            {
                Logger.Debug("TryInitializeSharedMemoryAsMaster: InitializeAsMaster failed (non-critical)");
                _sharedMemory.Dispose();
                _sharedMemory = null;
            }
            else
            {
                Logger.Info("Shared memory initialized - consumers can connect");
            }
        }
        catch (Exception ex)
        {
            Logger.Debug($"TryInitializeSharedMemoryAsMaster: Exception - {ex.Message} (non-critical)");
            _sharedMemory?.Dispose();
            _sharedMemory = null;
            // Non-critical - continue without shared memory
        }
    }

    /// <summary>
    /// Handle incoming motion data from COM port (master mode).
    /// </summary>
    private void OnMotionDataReceived(object? sender, OmniMotionData data)
    {
        // Log every 100th packet (debug only)
        if (_packetCount++ % 100 == 0)
        {
            Logger.Trace($"MotionData #{_packetCount}: RingAngle={data.RingAngle:F1}° GamePad=({data.GamePad_X},{data.GamePad_Y})");
        }
        
        if (!data.EnableRingAngle || !data.EnableGamePadData)
            return;
        
        // Process raw data - NO SMOOTHING here, let consumer/wrapper handle it
        // This prevents double-smoothing which causes lag
        float rawX = (data.GamePad_X - 127f) / 127f;
        float rawY = -(data.GamePad_Y - 127f) / 127f;
        
        rawX = ApplyDeadzone(rawX, _deadzone);
        rawY = ApplyDeadzone(rawY, _deadzone);
        
        rawX = Math.Clamp(rawX * _speedMultiplier, -1f, 1f);
        rawY = Math.Clamp(rawY * _speedMultiplier, -1f, 1f);
        
        // Pass through without smoothing - consumer/wrapper applies smoothing
        _lastX = rawX;
        _lastY = rawY;
        
        // Write to shared memory (if available)
        try
        {
            _sharedMemory?.WriteData(_lastX, _lastY, data.RingAngle, data.GamePad_X, data.GamePad_Y);
        }
        catch
        {
            // Shared memory write failed - non-critical, continue
        }
        
        // Invoke callback
        InvokeCallback(data.RingAngle, data.GamePad_X, data.GamePad_Y);
    }

    /// <summary>
    /// Consumer loop - reads from shared memory and monitors master health.
    /// </summary>
    private void ConsumerLoop()
    {
        int staleCount = 0;
        const int StaleCountThreshold = 10;
        int loopCount = 0;
        
        Logger.Debug("ConsumerLoop: Started");
        
        while (_running && _sharedMemory != null)
        {
            try
            {
                loopCount++;
                bool dataFresh = _sharedMemory.IsDataFresh(StaleDataThresholdMs);
                
                if (_sharedMemory.ReadData(out _, out _, out float yaw, out int rawX, out int rawY))
                {
                    // Log first 5 reads, then every 100
                    if (loopCount <= 5 || loopCount % 100 == 0)
                    {
                        Logger.Debug($"ConsumerLoop #{loopCount}: yaw={yaw:F1}, raw=({rawX},{rawY}), fresh={dataFresh}");
                    }
                    
                    if (dataFresh)
                    {
                        staleCount = 0;
                        InvokeCallback(yaw, rawX, rawY);
                    }
                    else
                    {
                        staleCount++;
                        
                        
                        if (staleCount >= StaleCountThreshold)
                        {
                            // Master appears dead - try failover
                            if (TryFailoverToMaster())
                            {
                                return; // Exit consumer loop
                            }
                            staleCount = 0;
                        }
                    }
                }
            }
            catch
            {
                // Error reading shared memory - try failover
                if (TryFailoverToMaster())
                {
                    return;
                }
            }
            
            Thread.Sleep(8); // ~120Hz (was 16ms/60Hz) - faster polling for responsive input
        }
    }

    /// <summary>
    /// Attempt to take over as master when previous master dies.
    /// </summary>
    private bool TryFailoverToMaster()
    {
        try
        {
            if (TreadmillSharedMemory.IsMasterRunning())
                return false; // Master is still alive
            
            // Clean up consumer state
            _sharedMemory?.Dispose();
            _sharedMemory = null;
            _isConsumer = false;
            
            // Try to become master
            return InitializeDirectMode(_comPort, _omniMode, _baudRate);
        }
        catch
        {
            return false;
        }
    }

    /// <summary>
    /// Invoke the registered callback safely.
    /// </summary>
    private int _callbackCount = 0;
    private void InvokeCallback(float ringAngle, int gamePadX, int gamePadY)
    {
        if (_callbackPtr == 0)
        {
            if (_callbackCount == 0)
                Logger.Debug("InvokeCallback: No callback registered!");
            return;
        }
        
        try
        {
            _callbackCount++;
            if (_callbackCount <= 5 || _callbackCount % 100 == 0)
            {
                Logger.Trace($"InvokeCallback #{_callbackCount}: yaw={ringAngle:F1}, pad=({gamePadX},{gamePadY})");
            }
            
            var callback = Marshal.GetDelegateForFunctionPointer<OmniDataCallback>(_callbackPtr);
            callback(ringAngle, gamePadX, gamePadY);
        }
        catch (Exception ex)
        {
            Logger.Debug($"InvokeCallback exception: {ex.Message}");
        }
    }

    private static float ApplyDeadzone(float value, float deadzone)
    {
        if (Math.Abs(value) < deadzone)
            return 0;
        
        float sign = value > 0 ? 1 : -1;
        return sign * (Math.Abs(value) - deadzone) / (1 - deadzone);
    }

    [UnmanagedCallersOnly(EntryPoint = "OmniReader_RegisterCallback")]
    public static void RegisterCallback(nint handle, nint callbackPtr)
    {
        var reader = GetReader(handle);
        reader._callbackPtr = callbackPtr;
    }

    [UnmanagedCallersOnly(EntryPoint = "OmniReader_Disconnect")]
    public static void Disconnect(nint handle)
    {
        var reader = GetReader(handle);
        reader._running = false;
        reader._consumerThread?.Join(1000);
        reader._handler?.Disconnect();
        reader._handler?.Dispose();
        reader._sharedMemory?.Dispose();
    }

    [UnmanagedCallersOnly(EntryPoint = "OmniReader_Destroy")]
    public static void Destroy(nint handle)
    {
        var gcHandle = GCHandle.FromIntPtr(handle);
        if (gcHandle.IsAllocated)
        {
            var reader = (MinimalOmniReader)gcHandle.Target!;
            reader._running = false;
            reader._consumerThread?.Join(1000);
            reader._handler?.Dispose();
            reader._sharedMemory?.Dispose();
            gcHandle.Free();
        }
    }

    private static MinimalOmniReader GetReader(nint handle)
    {
        var gcHandle = GCHandle.FromIntPtr(handle);
        return (MinimalOmniReader)gcHandle.Target!;
    }

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    private delegate void OmniDataCallback(float ringAngle, int gamePadX, int gamePadY);
}