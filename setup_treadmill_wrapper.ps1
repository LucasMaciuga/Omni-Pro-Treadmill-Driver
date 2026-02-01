# setup_treadmill_wrapper.ps1
# One-time setup script for TreadmillOpenVRWrapper
# 
# This script:
# 1. Finds SkyrimVR installation
# 2. Backs up the original openvr_api.dll
# 3. Copies it to the MO2 mod folder
#
# Usage: Run as Administrator (may be required for Steam folders)

param(
    [string]$MO2ModPath = "C:\Modlists\MGON\mods\Treadmill OpenVR Wrapper\Root",
    [string]$SkyrimVRPath = ""
)

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Treadmill OpenVR Wrapper Setup" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Find SkyrimVR if not specified
if (-not $SkyrimVRPath) {
    $possiblePaths = @(
        "C:\Program Files (x86)\Steam\steamapps\common\SkyrimVR",
        "D:\Steam\steamapps\common\SkyrimVR",
        "E:\Steam\steamapps\common\SkyrimVR",
        "E:\SteamLibrary\steamapps\common\SkyrimVR",
        "F:\Steam\steamapps\common\SkyrimVR",
        "D:\SteamLibrary\steamapps\common\SkyrimVR"
    )
    
    foreach ($path in $possiblePaths) {
        if (Test-Path "$path\SkyrimVR.exe") {
            $SkyrimVRPath = $path
            break
        }
    }
}

if (-not $SkyrimVRPath -or -not (Test-Path $SkyrimVRPath)) {
    Write-Host "ERROR: Could not find SkyrimVR installation!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please specify the path manually:" -ForegroundColor Yellow
    Write-Host '  .\setup_treadmill_wrapper.ps1 -SkyrimVRPath "D:\Games\SkyrimVR"'
    exit 1
}

Write-Host "Found SkyrimVR at: $SkyrimVRPath" -ForegroundColor Green
Write-Host "MO2 Mod folder:    $MO2ModPath" -ForegroundColor Green
Write-Host ""

$originalDll = "$SkyrimVRPath\openvr_api.dll"
$backupDll = "$SkyrimVRPath\openvr_api_original.dll"
$modBackupDll = "$MO2ModPath\openvr_api_original.dll"

# Check current state
Write-Host "Checking current state..." -ForegroundColor White

if (Test-Path $modBackupDll) {
    $modBackupInfo = Get-Item $modBackupDll
    Write-Host "  [OK] openvr_api_original.dll already in MO2 mod ($($modBackupInfo.Length) bytes)" -ForegroundColor Green
    Write-Host ""
    Write-Host "Setup already complete! Nothing to do." -ForegroundColor Green
    exit 0
}

if (-not (Test-Path $originalDll)) {
    Write-Host "  [ERROR] openvr_api.dll not found in SkyrimVR folder!" -ForegroundColor Red
    exit 1
}

$originalInfo = Get-Item $originalDll
Write-Host "  [FOUND] openvr_api.dll ($($originalInfo.Length) bytes, $($originalInfo.LastWriteTime))" -ForegroundColor Yellow

# Check if it's the original or already the wrapper
if ($originalInfo.Length -lt 100000) {
    Write-Host ""
    Write-Host "  [WARNING] This appears to be the wrapper DLL (too small)!" -ForegroundColor Yellow
    Write-Host "            Expected ~600KB for original, found $($originalInfo.Length) bytes" -ForegroundColor Yellow
    
    if (Test-Path $backupDll) {
        Write-Host "  [OK] Backup exists at: $backupDll" -ForegroundColor Green
        $backupInfo = Get-Item $backupDll
        
        # Copy backup to MO2 mod
        Write-Host ""
        Write-Host "Copying backup to MO2 mod folder..." -ForegroundColor White
        Copy-Item $backupDll -Destination $modBackupDll -Force
        Write-Host "  [OK] Copied to: $modBackupDll" -ForegroundColor Green
        
        Write-Host ""
        Write-Host "========================================" -ForegroundColor Cyan
        Write-Host "Setup complete!" -ForegroundColor Green
        Write-Host "========================================" -ForegroundColor Cyan
        exit 0
    } else {
        Write-Host "  [ERROR] No backup found! You may need to verify game files in Steam." -ForegroundColor Red
        exit 1
    }
}

# Original DLL found - copy to MO2 mod folder
Write-Host ""
Write-Host "Setting up..." -ForegroundColor White

# Copy original to MO2 mod folder as openvr_api_original.dll
Write-Host "  Copying original DLL to MO2 mod folder..." -ForegroundColor White
Copy-Item $originalDll -Destination $modBackupDll -Force
Write-Host "  [OK] Created: $modBackupDll" -ForegroundColor Green

# Also create backup in game folder (optional, for safety)
if (-not (Test-Path $backupDll)) {
    Write-Host "  Creating backup in game folder..." -ForegroundColor White
    Copy-Item $originalDll -Destination $backupDll -Force
    Write-Host "  [OK] Backup: $backupDll" -ForegroundColor Green
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Setup complete!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Your MO2 mod now contains:" -ForegroundColor Yellow
Get-ChildItem $MO2ModPath | Format-Table Name, Length -AutoSize
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "  1. Enable the mod in MO2"
Write-Host "  2. Start SteamVR (treadmill driver becomes master)"
Write-Host "  3. Start SkyrimVR through MO2"
Write-Host ""
