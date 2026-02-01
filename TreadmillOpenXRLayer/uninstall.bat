@echo off
:: ============================================================================
:: TreadmillOpenXRLayer - Uninstallation Script
:: ============================================================================
:: Run as Administrator!
:: ============================================================================

echo TreadmillOpenXRLayer Uninstaller
echo ================================
echo.

:: Check for admin rights
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo ERROR: This script requires Administrator privileges!
    echo Right-click and select "Run as administrator"
    pause
    exit /b 1
)

set INSTALL_DIR=%ProgramFiles%\TreadmillOpenXRLayer

echo Removing OpenXR layer registration...
reg delete "HKLM\SOFTWARE\Khronos\OpenXR\1\ApiLayers\Implicit" /v "%INSTALL_DIR%\XR_APILAYER_NOVENDOR_treadmill_locomotion.json" /f >nul 2>&1

echo Removing files...
if exist "%INSTALL_DIR%" (
    rmdir /S /Q "%INSTALL_DIR%"
    echo Removed installation directory
)

echo.
echo ============================================================================
echo Uninstallation complete!
echo ============================================================================
echo.
pause
