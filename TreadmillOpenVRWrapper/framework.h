// ============================================================================
// TreadmillOpenVRWrapper - OpenVR API Wrapper for Treadmill Input Injection
// ============================================================================
// This DLL replaces the game's openvr_api.dll and intercepts input calls
// to inject treadmill movement data.
//
// Installation:
// 1. Rename original openvr_api.dll to openvr_api_original.dll
// 2. Copy this DLL as openvr_api.dll
// 3. Copy OmniBridge.dll and treadmill_config.json to game folder
//
// Supported games:
// - SkyrimVR (including MGON/VRIK)
// - Fallout 4 VR
// - Any OpenVR game
// ============================================================================
#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <atomic>
#include <mutex>
#include <string>
#include <vector>
#include <functional>
#include <cmath>
#include <fstream>
#include <filesystem>
