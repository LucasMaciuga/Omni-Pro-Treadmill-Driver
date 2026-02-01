using System.IO.MemoryMappedFiles;
using System.Runtime.InteropServices;
using System.Diagnostics;

namespace OmniBridge;

/// <summary>
/// Shared Memory implementation for multi-process treadmill data sharing.
/// 
/// Architecture:
/// - First process to call Initialize() becomes the MASTER
/// - Master opens COM port and writes to shared memory
/// - Subsequent processes become CONSUMERS and read from shared memory
/// - When master exits, next process to initialize can become master
/// </summary>
public class TreadmillSharedMemory : IDisposable
{
    // Use Local\ namespace instead of Global\ to avoid admin requirement
    // This means shared memory only works within the same user session
    private const string SharedMemoryName = "Local\\OmniTreadmillData";
    private const string MutexName = "Local\\OmniTreadmillMutex";
    private const uint Magic = 0x4F4D4E49; // 'OMNI'
    private const uint Version = 1;

    // Use a simple struct without managed types for blittable compatibility
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct SharedData
    {
        public uint Magic;
        public uint Version;
        public float X;
        public float Y;
        public float Yaw;
        public int RawX;
        public int RawY;
        public uint Connected;
        public long LastUpdate;
        public long UpdateCount;
        public uint MasterPid;
        // Reserved space as individual fields (no managed array)
        public long Reserved1;
        public long Reserved2;
        public long Reserved3;
        public long Reserved4;
    }
    
    private static readonly int SharedDataSize = Marshal.SizeOf<SharedData>();

    private MemoryMappedFile? _mappedFile;
    private MemoryMappedViewAccessor? _accessor;
    private Mutex? _mutex;
    private bool _isMaster;
    private bool _disposed;


    public bool IsMaster => _isMaster;
    public bool IsConnected => _accessor != null;

    /// <summary>
    /// Check if a master process is already running
    /// </summary>
    public static bool IsMasterRunning()
    {
        try
        {
            using var testFile = MemoryMappedFile.OpenExisting(SharedMemoryName, MemoryMappedFileRights.Read);
            using var testAccessor = testFile.CreateViewAccessor(0, SharedDataSize, MemoryMappedFileAccess.Read);
            
            SharedData data;
            testAccessor.Read(0, out data);
            
            if (data.Magic != Magic)
                return false;
            
            // Check if master process is still alive
            try
            {
                using var process = Process.GetProcessById((int)data.MasterPid);
                return !process.HasExited;
            }
            catch
            {
                // Process not found or access denied
                return false;
            }
        }
        catch
        {
            // Shared memory doesn't exist
            return false;
        }
    }

    /// <summary>
    /// Initialize as master (creates shared memory)
    /// </summary>
    public bool InitializeAsMaster()
    {
        try
        {
            Logger.Debug($"InitializeAsMaster: Creating mutex '{MutexName}'...");
            _mutex = new Mutex(false, MutexName);
            
            Logger.Debug($"InitializeAsMaster: Creating shared memory '{SharedMemoryName}' (size={SharedDataSize})...");
            _mappedFile = MemoryMappedFile.CreateOrOpen(
                SharedMemoryName,
                SharedDataSize,
                MemoryMappedFileAccess.ReadWrite);
            
            Logger.Debug("InitializeAsMaster: Creating view accessor...");
            _accessor = _mappedFile.CreateViewAccessor(0, SharedDataSize, MemoryMappedFileAccess.ReadWrite);
            
            // Initialize shared data
            var pid = (uint)Process.GetCurrentProcess().Id;
            var data = new SharedData
            {
                Magic = Magic,
                Version = Version,
                X = 0,
                Y = 0,
                Yaw = 0,
                RawX = 127,
                RawY = 127,
                Connected = 0,
                LastUpdate = 0,
                UpdateCount = 0,
                MasterPid = pid,
                Reserved1 = 0,
                Reserved2 = 0,
                Reserved3 = 0,
                Reserved4 = 0
            };
            
            Logger.Debug($"InitializeAsMaster: Writing initial data (PID={pid})...");
            _accessor.Write(0, ref data);
            _isMaster = true;
            
            Logger.Info($"Shared memory master ready (PID {pid})");
            return true;
        }
        catch (Exception ex)
        {
            Logger.Error("InitializeAsMaster failed", ex);
            return false;
        }
    }

    /// <summary>
    /// Initialize as consumer (connects to existing shared memory)
    /// </summary>
    public bool InitializeAsConsumer()
    {
        try
        {
            Logger.Debug("InitializeAsConsumer: Opening existing shared memory...");
            _mappedFile = MemoryMappedFile.OpenExisting(SharedMemoryName, MemoryMappedFileRights.Read);
            _accessor = _mappedFile.CreateViewAccessor(0, SharedDataSize, MemoryMappedFileAccess.Read);
            
            SharedData data;
            _accessor.Read(0, out data);
            
            if (data.Magic != Magic)
            {
                Logger.Error($"InitializeAsConsumer: Invalid magic 0x{data.Magic:X8}");
                return false;
            }
            
            _isMaster = false;
            Logger.Info($"Shared memory consumer connected (master PID {data.MasterPid})");
            return true;
        }
        catch (Exception ex)
        {
            Logger.Error("InitializeAsConsumer failed", ex);
            return false;
        }
    }

    /// <summary>
    /// Write treadmill data (master only)
    /// </summary>
    public void WriteData(float x, float y, float yaw, int rawX, int rawY)
    {
        if (!_isMaster || _accessor == null)
            return;
        
        SharedData data;
        _accessor.Read(0, out data);
        
        data.X = x;
        data.Y = y;
        data.Yaw = yaw;
        data.RawX = rawX;
        data.RawY = rawY;
        data.Connected = 1;
        data.LastUpdate = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds();
        data.UpdateCount++;
        
        _accessor.Write(0, ref data);
    }

    /// <summary>
    /// Read treadmill data (consumer mode)
    /// </summary>
    public bool ReadData(out float x, out float y, out float yaw, out int rawX, out int rawY)
    {
        x = 0; y = 0; yaw = 0; rawX = 127; rawY = 127;
        
        if (_accessor == null)
            return false;
        
        SharedData data;
        _accessor.Read(0, out data);
        
        x = data.X;
        y = data.Y;
        yaw = data.Yaw;
        rawX = data.RawX;
        rawY = data.RawY;
        
        return data.Connected != 0;
    }

    /// <summary>
    /// Check if data is fresh (updated within maxAgeMs)
    /// </summary>
    public bool IsDataFresh(int maxAgeMs = 500)
    {
        if (_accessor == null)
            return false;
        
        SharedData data;
        _accessor.Read(0, out data);
        
        long now = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds();
        return (now - data.LastUpdate) < maxAgeMs;
    }

    /// <summary>
    /// Mark as disconnected (master only)
    /// </summary>
    public void SetDisconnected()
    {
        if (!_isMaster || _accessor == null)
            return;
        
        SharedData data;
        _accessor.Read(0, out data);
        data.Connected = 0;
        data.MasterPid = 0;
        _accessor.Write(0, ref data);
    }

    public void Dispose()
    {
        if (_disposed)
            return;
        
        if (_isMaster)
        {
            SetDisconnected();
        }
        
        _accessor?.Dispose();
        _mappedFile?.Dispose();
        _mutex?.Dispose();
        
        _accessor = null;
        _mappedFile = null;
        _mutex = null;
        _disposed = true;
    }
}
