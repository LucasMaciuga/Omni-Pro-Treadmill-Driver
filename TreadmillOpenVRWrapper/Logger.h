// ============================================================================
// TreadmillOpenVRWrapper - Logger
// ============================================================================
// Centralized logging with build-type awareness:
// - Release: Only INFO and ERROR are logged
// - Debug: All levels including DEBUG and TRACE
// ============================================================================
#pragma once

#include <string>

namespace TreadmillWrapper {

// ============================================================================
// LOG LEVELS
// ============================================================================

enum class LogLevel {
    Trace,   // Very detailed, frequent (every packet)
    Debug,   // Detailed debugging info
    Info,    // Important status messages
    Error    // Errors only
};

// ============================================================================
// LOGGER CLASS
// ============================================================================

class Logger {
public:
    /// Initialize logging to file
    static void Init(const std::wstring& logPath);
    
    /// Shutdown and close log file
    static void Shutdown();
    
    /// Set minimum log level (default: Info for Release, Debug for Debug build)
    static void SetMinLevel(LogLevel level);
    
    /// Enable/disable debug logging at runtime
    static void SetDebugEnabled(bool enabled);
    
    /// Check if debug logging is enabled
    static bool IsDebugEnabled();

    /// Log at specific level (internal use)
    static void Write(LogLevel level, const char* format, ...);
};

// ============================================================================
// CONVENIENCE FUNCTIONS
// ============================================================================

/// Log important info (always logged in both Debug and Release)
/// Use for: startup, shutdown, successful connections, mode changes
void LogInfo(const char* format, ...);

/// Log errors (always logged in both Debug and Release)
void LogError(const char* format, ...);

/// Log debug details (only in Debug build OR when debugLog=true in config)
/// Use for: detailed state, configuration, occasional updates
void LogDebug(const char* format, ...);

/// Log verbose/trace details (only in Debug build AND debugLog=true)
/// Use for: every packet, raw data, very frequent updates
void LogTrace(const char* format, ...);

/// Legacy Log function (maps to LogDebug for compatibility)
void Log(const char* format, ...);

/// Initialize logging
void InitLogging(const std::wstring& logPath);

/// Shutdown logging
void ShutdownLogging();

} // namespace TreadmillWrapper
