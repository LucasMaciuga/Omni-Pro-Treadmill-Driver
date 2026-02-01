using System.Runtime.InteropServices;
using System.Diagnostics;

namespace OmniBridge;

/// <summary>
/// Centralized logging for OmniBridge.
/// - Release: Only startup info and errors are logged
/// - Debug: Full detailed logging
/// </summary>
public static class Logger
{
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
    private static extern void OutputDebugStringW(string message);
    
    private static readonly string LogPath = Path.Combine(Path.GetTempPath(), "OmniBridge.log");
    private static readonly object LockObj = new();
    
    /// <summary>
    /// Log important info (always logged in both Debug and Release).
    /// Use for: startup, shutdown, successful connections, mode changes.
    /// </summary>
    public static void Info(string message)
    {
        WriteLog("INFO", message);
    }
    
    /// <summary>
    /// Log errors (always logged in both Debug and Release).
    /// </summary>
    public static void Error(string message)
    {
        WriteLog("ERROR", message);
    }
    
    /// <summary>
    /// Log errors with exception details (always logged).
    /// </summary>
    public static void Error(string message, Exception ex)
    {
        WriteLog("ERROR", $"{message}: {ex.GetType().Name} - {ex.Message}");
    }
    
    /// <summary>
    /// Log debug details (only in Debug build).
    /// Use for: packet counts, detailed state, periodic updates.
    /// </summary>
    [Conditional("DEBUG")]
    public static void Debug(string message)
    {
        WriteLog("DEBUG", message);
    }
    
    /// <summary>
    /// Log verbose/trace details (only in Debug build).
    /// Use for: every packet, raw data, frequent updates.
    /// </summary>
    [Conditional("DEBUG")]
    public static void Trace(string message)
    {
        WriteLog("TRACE", message);
    }
    
    private static void WriteLog(string level, string message)
    {
        string formatted = $"[OmniBridge:{level}] {message}";
        
        // Always output to debug stream
        OutputDebugStringW(formatted + "\n");
        
        // Write to file
        try
        {
            lock (LockObj)
            {
                File.AppendAllText(LogPath, $"{DateTime.Now:HH:mm:ss.fff} [{level}] {message}\n");
            }
        }
        catch { }
    }
}
