// ============================================================================
// TreadmillOpenVRWrapper - Logger Implementation
// ============================================================================
#include "pch.h"
#include "Logger.h"
#include <cstdarg>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <mutex>
#include <Windows.h>

namespace TreadmillWrapper {

// ============================================================================
// STATIC STATE
// ============================================================================

static std::ofstream g_logFile;
static std::mutex g_logMutex;
static bool g_debugEnabled = false;
static LogLevel g_minLevel = LogLevel::Info;

// ============================================================================
// LOGGER CLASS IMPLEMENTATION
// ============================================================================

void Logger::Init(const std::wstring& logPath) {
    std::lock_guard<std::mutex> lock(g_logMutex);
    
    g_logFile.open(logPath, std::ios::out | std::ios::trunc);
    if (g_logFile.is_open()) {
        g_logFile << "========================================\n";
        g_logFile << "TreadmillOpenVRWrapper Log\n";
#ifdef _DEBUG
        g_logFile << "Build: DEBUG\n";
        g_minLevel = LogLevel::Debug;
#else
        g_logFile << "Build: RELEASE\n";
        g_minLevel = LogLevel::Info;
#endif
        g_logFile << "========================================\n";
        g_logFile.flush();
    }
}

void Logger::Shutdown() {
    std::lock_guard<std::mutex> lock(g_logMutex);
    
    if (g_logFile.is_open()) {
        g_logFile << "========================================\n";
        g_logFile << "Log closed\n";
        g_logFile.close();
    }
}

void Logger::SetMinLevel(LogLevel level) {
    g_minLevel = level;
}

void Logger::SetDebugEnabled(bool enabled) {
    g_debugEnabled = enabled;
}

bool Logger::IsDebugEnabled() {
    return g_debugEnabled;
}

static const char* LevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Trace: return "TRACE";
        case LogLevel::Debug: return "DEBUG";
        case LogLevel::Info:  return "INFO";
        case LogLevel::Error: return "ERROR";
        default: return "???";
    }
}

void Logger::Write(LogLevel level, const char* format, ...) {
    // Build level string
    const char* levelStr = LevelToString(level);
    
    // Format message
    char buffer[2048];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    std::lock_guard<std::mutex> lock(g_logMutex);
    
    // Write to file
    if (g_logFile.is_open()) {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        struct tm timeinfo;
        localtime_s(&timeinfo, &time);
        
        char timeBuf[64];
        strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", &timeinfo);
        
        g_logFile << "[" << timeBuf << "." 
                  << std::setfill('0') << std::setw(3) << ms.count() 
                  << "] [" << levelStr << "] " << buffer << "\n";
        g_logFile.flush();
    }
    
    // Write to debug output
    char debugStr[2048];
    snprintf(debugStr, sizeof(debugStr), "[TreadmillWrapper:%s] %s\n", levelStr, buffer);
    OutputDebugStringA(debugStr);
}

// ============================================================================
// CONVENIENCE FUNCTIONS
// ============================================================================

void LogInfo(const char* format, ...) {
    char buffer[2048];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    Logger::Write(LogLevel::Info, "%s", buffer);
}

void LogError(const char* format, ...) {
    char buffer[2048];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    Logger::Write(LogLevel::Error, "%s", buffer);
}

void LogDebug(const char* format, ...) {
#ifdef _DEBUG
    // In Debug build: always log
    char buffer[2048];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    Logger::Write(LogLevel::Debug, "%s", buffer);
#else
    // In Release build: only log if debug is enabled
    if (!Logger::IsDebugEnabled()) return;
    
    char buffer[2048];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    Logger::Write(LogLevel::Debug, "%s", buffer);
#endif
}

void LogTrace(const char* format, ...) {
#ifdef _DEBUG
    // In Debug build: log if debug is enabled
    if (!Logger::IsDebugEnabled()) return;
    
    char buffer[2048];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    Logger::Write(LogLevel::Trace, "%s", buffer);
#else
    // In Release build: never log trace
    (void)format;
#endif
}

void Log(const char* format, ...) {
    // Legacy function - maps to LogDebug
    char buffer[2048];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
#ifdef _DEBUG
    Logger::Write(LogLevel::Debug, "%s", buffer);
#else
    if (Logger::IsDebugEnabled()) {
        Logger::Write(LogLevel::Debug, "%s", buffer);
    }
#endif
}

void InitLogging(const std::wstring& logPath) {
    Logger::Init(logPath);
}

void ShutdownLogging() {
    Logger::Shutdown();
}

} // namespace TreadmillWrapper
