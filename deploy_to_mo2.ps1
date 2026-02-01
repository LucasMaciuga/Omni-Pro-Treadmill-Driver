# deploy_to_mo2.ps1
# Deploys the TreadmillOpenVRWrapper and OmniBridge to MO2 mod folder
#
# Usage: .\deploy_to_mo2.ps1
#
# Prerequisites:
# 1. Build TreadmillOpenVRWrapper in Visual Studio (Release x64)
# 2. Build OmniBridge: dotnet publish -c Release -o publish

param(
    [string]$MO2ModPath = "C:\Modlists\MGON\mods\Treadmill OpenVR Wrapper\Root"
)

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Deploy to MO2 Mod Folder" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Target: $MO2ModPath" -ForegroundColor Yellow
Write-Host ""

# Source paths
$wrapperSource = "x64\Release\TreadmillOpenVRWrapper.dll"
$omniBridgeSource = "OmniBridge\publish\OmniBridge.dll"
$configSource = "..\TreadmillOpenVRWrapper\treadmill_config.json"

# Check if MO2 mod folder exists
if (-not (Test-Path $MO2ModPath)) {
    Write-Host "Creating mod folder: $MO2ModPath" -ForegroundColor Yellow
    New-Item -ItemType Directory -Path $MO2ModPath -Force | Out-Null
}

# Deploy Wrapper
Write-Host "Deploying Wrapper..." -ForegroundColor White
if (Test-Path $wrapperSource) {
    $wrapperInfo = Get-Item $wrapperSource
    Copy-Item $wrapperSource -Destination "$MO2ModPath\openvr_api.dll" -Force
    Write-Host "  [OK] openvr_api.dll ($($wrapperInfo.Length) bytes, $($wrapperInfo.LastWriteTime))" -ForegroundColor Green
} else {
    Write-Host "  [SKIP] Wrapper not found at $wrapperSource" -ForegroundColor Yellow
    Write-Host "         Build TreadmillOpenVRWrapper in Visual Studio first!" -ForegroundColor Yellow
}

# Deploy OmniBridge
Write-Host "Deploying OmniBridge..." -ForegroundColor White
if (Test-Path $omniBridgeSource) {
    $bridgeInfo = Get-Item $omniBridgeSource
    Copy-Item $omniBridgeSource -Destination $MO2ModPath -Force
    Write-Host "  [OK] OmniBridge.dll ($($bridgeInfo.Length) bytes, $($bridgeInfo.LastWriteTime))" -ForegroundColor Green
} else {
    Write-Host "  [SKIP] OmniBridge not found at $omniBridgeSource" -ForegroundColor Yellow
    Write-Host "         Run: dotnet publish -c Release -o publish" -ForegroundColor Yellow
}

# Deploy Config (only if not exists or force)
Write-Host "Checking config..." -ForegroundColor White
$configDest = "$MO2ModPath\treadmill_config.json"
if (-not (Test-Path $configDest)) {
    if (Test-Path $configSource) {
        Copy-Item $configSource -Destination $configDest
        Write-Host "  [OK] treadmill_config.json (created)" -ForegroundColor Green
    }
} else {
    Write-Host "  [SKIP] treadmill_config.json already exists (not overwritten)" -ForegroundColor Gray
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Deployment complete!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Current mod contents:" -ForegroundColor Yellow
Get-ChildItem $MO2ModPath | Format-Table Name, Length, LastWriteTime -AutoSize
