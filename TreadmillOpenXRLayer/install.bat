@echo off
:: ============================================================================
:: TreadmillOpenXRLayer - Installation Script
:: ============================================================================
:: Run as Administrator!
:: ============================================================================

echo TreadmillOpenXRLayer Installer
echo ==============================
echo.

:: Check for admin rights
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo ERROR: This script requires Administrator privileges!
    echo Right-click and select "Run as administrator"
    pause
    exit /b 1
)

:: Set installation directory
set INSTALL_DIR=%ProgramFiles%\TreadmillOpenXRLayer

echo Installing to: %INSTALL_DIR%
echo.

:: Create installation directory
if not exist "%INSTALL_DIR%" (
    mkdir "%INSTALL_DIR%"
    echo Created installation directory
)

:: Copy files
echo Copying files...
copy /Y TreadmillOpenXRLayer.dll "%INSTALL_DIR%\" >nul
copy /Y XR_APILAYER_NOVENDOR_treadmill_locomotion.json "%INSTALL_DIR%\" >nul
copy /Y treadmill_layer_config.json "%INSTALL_DIR%\" >nul
copy /Y OmniBridge.dll "%INSTALL_DIR%\" >nul 2>nul

:: Update manifest with full path
echo Updating manifest...
powershell -Command "(Get-Content '%INSTALL_DIR%\XR_APILAYER_NOVENDOR_treadmill_locomotion.json') -replace '\\.\\\\', '%INSTALL_DIR:\=\\%\\' | Set-Content '%INSTALL_DIR%\XR_APILAYER_NOVENDOR_treadmill_locomotion.json'"

:: Register in registry
echo Registering OpenXR layer...
reg add "HKLM\SOFTWARE\Khronos\OpenXR\1\ApiLayers\Implicit" /v "%INSTALL_DIR%\XR_APILAYER_NOVENDOR_treadmill_locomotion.json" /t REG_DWORD /d 0 /f >nul

echo.
echo ============================================================================
echo Installation complete!
echo ============================================================================
echo.
echo The layer is now active for all OpenXR applications.
echo.
echo Configuration file: %INSTALL_DIR%\treadmill_layer_config.json
echo Log file: %INSTALL_DIR%\treadmill_layer.log
echo.
echo To disable temporarily, set environment variable: DISABLE_TREADMILL_LAYER=1
echo.
pause
