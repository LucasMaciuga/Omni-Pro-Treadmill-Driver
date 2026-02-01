# create_mo2_package.ps1
# Creates an importable MO2 mod package for TreadmillOpenVRWrapper
#
# Usage: .\create_mo2_package.ps1 [-Version "1.0.0"]

param(
    [string]$Version = "1.0.0",
    [string]$OutputDir = ".\releases"
)

$ErrorActionPreference = "Stop"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Creating MO2 Mod Package" -ForegroundColor Cyan
Write-Host "Version: $Version" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# Paths
$wrapperDll = "x64\Release\TreadmillOpenVRWrapper.dll"
$omniBridgeDll = "OmniBridge\publish\OmniBridge.dll"
$configFile = "..\TreadmillOpenVRWrapper\treadmill_config.json"
$readmeSource = "..\TreadmillOpenVRWrapper\README.md"

# Check files exist
$missingFiles = @()
if (-not (Test-Path $wrapperDll)) { $missingFiles += "Wrapper DLL: $wrapperDll" }
if (-not (Test-Path $omniBridgeDll)) { $missingFiles += "OmniBridge DLL: $omniBridgeDll" }
if (-not (Test-Path $configFile)) { $missingFiles += "Config: $configFile" }

if ($missingFiles.Count -gt 0) {
    Write-Host "ERROR: Missing files:" -ForegroundColor Red
    $missingFiles | ForEach-Object { Write-Host "  - $_" -ForegroundColor Red }
    Write-Host ""
    Write-Host "Build the projects first:" -ForegroundColor Yellow
    Write-Host "  1. Build TreadmillOpenVRWrapper in Visual Studio (Release x64)"
    Write-Host "  2. Run: dotnet publish OmniBridge -c Release -o OmniBridge/publish"
    exit 1
}

# Create output directory
$packageName = "TreadmillOpenVRWrapper-v$Version"
$tempDir = "$env:TEMP\$packageName"
$rootDir = "$tempDir\Root"

if (Test-Path $tempDir) { Remove-Item $tempDir -Recurse -Force }
New-Item -ItemType Directory -Path $rootDir -Force | Out-Null

# Copy files
Write-Host ""
Write-Host "Copying files..." -ForegroundColor White

Copy-Item $wrapperDll -Destination "$rootDir\openvr_api.dll"
Write-Host "  [OK] openvr_api.dll" -ForegroundColor Green

Copy-Item $omniBridgeDll -Destination "$rootDir\OmniBridge.dll"
Write-Host "  [OK] OmniBridge.dll" -ForegroundColor Green

Copy-Item $configFile -Destination "$rootDir\treadmill_config.json"
Write-Host "  [OK] treadmill_config.json" -ForegroundColor Green

# Create installation instructions
$instructions = @"
# Treadmill OpenVR Wrapper v$Version

## Quick Setup (Recommended)

1. Open PowerShell in this mod's folder
2. Run: .\setup.ps1
3. Done! The script handles everything automatically.

## Manual Installation

### IMPORTANT: Before Using
You must have the original openvr_api.dll backed up!

1. Go to your SkyrimVR folder (e.g., Steam\steamapps\common\SkyrimVR)
2. Find openvr_api.dll (~600KB)
3. Copy it to this mod's Root folder
4. Rename the copy to openvr_api_original.dll

### Requirements
- SteamVR with Treadmill driver installed and running
- Virtuix Omni treadmill connected

### How It Works
1. Start SteamVR first (treadmill driver becomes "master")
2. Start SkyrimVR through MO2 (wrapper connects as "consumer")
3. Walk on your treadmill - movement is injected into the game!

### Configuration
Edit Root\treadmill_config.json to adjust:
- comPort: Your treadmill's COM port (default: COM3)
- speedMultiplier: Movement speed (default: 1.5)
- deadzone: Ignore small movements (default: 0.1)
- inputMode: override/additive/smart (default: smart)

### Troubleshooting
- Check Root\treadmill_wrapper.log for errors
- Make sure SteamVR treadmill driver shows "connected"
- Verify COM port in Device Manager
"@

$instructions | Out-File "$tempDir\README.txt" -Encoding UTF8
Write-Host "  [OK] README.txt" -ForegroundColor Green

# Create setup script for the package
$setupScript = @'
# setup.ps1 - Automatic setup for TreadmillOpenVRWrapper
# Run this script once after installing the mod in MO2

$ErrorActionPreference = "Stop"
$modRoot = "$PSScriptRoot\Root"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Treadmill OpenVR Wrapper Setup" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# Check if already set up
if (Test-Path "$modRoot\openvr_api_original.dll") {
    Write-Host "Already set up! openvr_api_original.dll exists." -ForegroundColor Green
    exit 0
}

# Find SkyrimVR
$paths = @(
    "C:\Program Files (x86)\Steam\steamapps\common\SkyrimVR",
    "D:\Steam\steamapps\common\SkyrimVR",
    "E:\Steam\steamapps\common\SkyrimVR", 
    "E:\SteamLibrary\steamapps\common\SkyrimVR",
    "F:\SteamLibrary\steamapps\common\SkyrimVR"
)

$skyrimPath = $null
foreach ($p in $paths) {
    if (Test-Path "$p\openvr_api.dll") { $skyrimPath = $p; break }
}

if (-not $skyrimPath) {
    Write-Host "ERROR: Could not find SkyrimVR!" -ForegroundColor Red
    Write-Host "Please copy openvr_api.dll manually from your SkyrimVR folder" -ForegroundColor Yellow
    Write-Host "to: $modRoot\openvr_api_original.dll" -ForegroundColor Yellow
    exit 1
}

Write-Host "Found SkyrimVR at: $skyrimPath" -ForegroundColor Green

$original = "$skyrimPath\openvr_api.dll"
$info = Get-Item $original

if ($info.Length -lt 100000) {
    Write-Host "WARNING: openvr_api.dll seems to be a wrapper already!" -ForegroundColor Yellow
    if (Test-Path "$skyrimPath\openvr_api_original.dll") {
        $original = "$skyrimPath\openvr_api_original.dll"
        Write-Host "Using existing backup: $original" -ForegroundColor Green
    } else {
        Write-Host "No backup found. Please verify game files in Steam first." -ForegroundColor Red
        exit 1
    }
}

Write-Host "Copying original DLL to mod folder..." -ForegroundColor White
Copy-Item $original -Destination "$modRoot\openvr_api_original.dll" -Force

Write-Host ""
Write-Host "Setup complete!" -ForegroundColor Green
Write-Host "Enable the mod in MO2 and enjoy!" -ForegroundColor Cyan
'@

$setupScript | Out-File "$tempDir\setup.ps1" -Encoding UTF8
Write-Host "  [OK] setup.ps1" -ForegroundColor Green

# Create meta.ini for MO2
$metaIni = @"
[General]
modid=0
version=$Version
newestVersion=
category=42
installationFile=
gameName=SkyrimVR
comments=Virtuix Omni Treadmill Support for VR Games
notes=Requires openvr_api_original.dll to be added manually!
url=https://github.com/LucasMaci/Omni-Pro-Treadmill-Driver

[installedFiles]
size=0
"@

$metaIni | Out-File "$tempDir\meta.ini" -Encoding UTF8
Write-Host "  [OK] meta.ini" -ForegroundColor Green

# Create ZIP
if (-not (Test-Path $OutputDir)) { New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null }
$zipPath = "$OutputDir\$packageName.zip"

if (Test-Path $zipPath) { Remove-Item $zipPath -Force }

Write-Host ""
Write-Host "Creating ZIP archive..." -ForegroundColor White
Compress-Archive -Path "$tempDir\*" -DestinationPath $zipPath -CompressionLevel Optimal

# Cleanup
Remove-Item $tempDir -Recurse -Force

# Show result
$zipInfo = Get-Item $zipPath
Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Package created successfully!" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Output: $($zipInfo.FullName)" -ForegroundColor Yellow
Write-Host "Size:   $([math]::Round($zipInfo.Length / 1MB, 2)) MB" -ForegroundColor Yellow
Write-Host ""
Write-Host "Contents:" -ForegroundColor White
Write-Host "  - Root/openvr_api.dll (Wrapper)"
Write-Host "  - Root/OmniBridge.dll (Treadmill communication)"
Write-Host "  - Root/treadmill_config.json (Configuration)"
Write-Host "  - README.txt (Installation instructions)"
Write-Host "  - setup.ps1 (Automatic setup script)"
Write-Host "  - meta.ini (MO2 metadata)"
Write-Host ""
Write-Host "Users run setup.ps1 once to complete installation!" -ForegroundColor Yellow
