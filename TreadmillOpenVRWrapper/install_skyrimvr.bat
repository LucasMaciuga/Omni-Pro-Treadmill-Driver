@echo off
:: ============================================================================
:: TreadmillOpenVRWrapper - Installation Script for SkyrimVR
:: ============================================================================

echo TreadmillOpenVRWrapper Installer for SkyrimVR
echo ==============================================
echo.

:: Find the wrapper DLL
set SCRIPT_DIR=%~dp0
set BUILD_DIR=%SCRIPT_DIR%..\Omni-Pro-Treadmill-Driver\x64\Release

:: Check if build exists
if exist "%BUILD_DIR%\TreadmillOpenVRWrapper.dll" (
    set WRAPPER_DLL=%BUILD_DIR%\TreadmillOpenVRWrapper.dll
    echo Found wrapper DLL at: %WRAPPER_DLL%
) else if exist "%SCRIPT_DIR%TreadmillOpenVRWrapper.dll" (
    set WRAPPER_DLL=%SCRIPT_DIR%TreadmillOpenVRWrapper.dll
    echo Found wrapper DLL at: %WRAPPER_DLL%
) else if exist "%SCRIPT_DIR%openvr_api.dll" (
    set WRAPPER_DLL=%SCRIPT_DIR%openvr_api.dll
    echo Found wrapper DLL at: %WRAPPER_DLL%
) else (
    echo ERROR: Cannot find TreadmillOpenVRWrapper.dll!
    echo Please build the project first or copy the DLL to this folder.
    pause
    exit /b 1
)

:: Default Steam path
set SKYRIM_PATH=C:\Program Files (x86)\Steam\steamapps\common\SkyrimVR

:: Check if path exists
if not exist "%SKYRIM_PATH%\SkyrimVR.exe" (
    echo SkyrimVR not found at default location.
    set /p SKYRIM_PATH=Please enter the full path to your SkyrimVR folder: 
)

if not exist "%SKYRIM_PATH%\SkyrimVR.exe" (
    echo ERROR: SkyrimVR.exe not found in specified path!
    pause
    exit /b 1
)

echo.
echo Installing to: %SKYRIM_PATH%
echo.

:: Backup original DLL
if exist "%SKYRIM_PATH%\openvr_api.dll" (
    if not exist "%SKYRIM_PATH%\openvr_api_original.dll" (
        echo Backing up original openvr_api.dll...
        copy /Y "%SKYRIM_PATH%\openvr_api.dll" "%SKYRIM_PATH%\openvr_api_original.dll" >nul
        echo   [OK] Original backed up to openvr_api_original.dll
    ) else (
        echo   [OK] Original DLL already backed up.
    )
)

:: Copy wrapper as openvr_api.dll
echo Installing wrapper DLL...
copy /Y "%WRAPPER_DLL%" "%SKYRIM_PATH%\openvr_api.dll" >nul
echo   [OK] Wrapper installed as openvr_api.dll

:: Copy config file
echo Installing configuration...
copy /Y "%SCRIPT_DIR%treadmill_config.json" "%SKYRIM_PATH%\" >nul
echo   [OK] Configuration installed

:: Copy OmniBridge.dll if exists
if exist "%BUILD_DIR%\OmniBridge.dll" (
    copy /Y "%BUILD_DIR%\OmniBridge.dll" "%SKYRIM_PATH%\" >nul
    echo   [OK] OmniBridge.dll installed
) else if exist "%SCRIPT_DIR%OmniBridge.dll" (
    copy /Y "%SCRIPT_DIR%OmniBridge.dll" "%SKYRIM_PATH%\" >nul
    echo   [OK] OmniBridge.dll installed
) else (
    echo   [!] WARNING: OmniBridge.dll not found!
    echo       You need to copy it manually to %SKYRIM_PATH%
)

echo.
echo ============================================================================
echo Installation complete!
echo ============================================================================
echo.
echo Files installed:
echo   %SKYRIM_PATH%\openvr_api.dll (wrapper)
echo   %SKYRIM_PATH%\openvr_api_original.dll (original, backup)
echo   %SKYRIM_PATH%\treadmill_config.json (configuration)
echo.
echo IMPORTANT: Edit treadmill_config.json to set your COM port!
echo.
echo To uninstall:
echo   1. Delete openvr_api.dll
echo   2. Rename openvr_api_original.dll to openvr_api.dll
echo.
pause
