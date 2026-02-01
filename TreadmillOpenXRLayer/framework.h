// ============================================================================
// TreadmillOpenXRLayer - OpenXR API Layer for Treadmill Input Injection
// ============================================================================
// This is an OpenXR API Layer that intercepts input calls to inject
// treadmill movement data into any OpenXR application.
//
// Installation:
// 1. Register the layer via registry or environment variable
// 2. The layer automatically loads for all OpenXR applications
//
// Supported:
// - The Midnight Walk
// - UEVR (all Unreal Engine games)
// - Any OpenXR game/application
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
#include <unordered_map>
