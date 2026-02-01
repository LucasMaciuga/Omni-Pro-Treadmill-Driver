@echo off
REM start_skyrimvr.bat - Wrapper script for SkyrimVR with Treadmill support
REM Place this in your SkyrimVR folder or run from MO2

SET SKYRIM_PATH=E:\SteamLibrary\steamapps\common\SkyrimVR
SET ORIGINAL_DLL=%SKYRIM_PATH%\openvr_api.dll
SET BACKUP_DLL=%SKYRIM_PATH%\openvr_api_original.dll

REM Check if backup exists, if not create it
if not exist "%BACKUP_DLL%" (
    if exist "%ORIGINAL_DLL%" (
        echo Creating backup of openvr_api.dll...
        copy "%ORIGINAL_DLL%" "%BACKUP_DLL%"
        echo Backup created!
    )
)

REM Start SkyrimVR (MO2 should handle the wrapper DLL injection)
echo Starting SkyrimVR...
start "" "%SKYRIM_PATH%\SkyrimVR.exe" %*
