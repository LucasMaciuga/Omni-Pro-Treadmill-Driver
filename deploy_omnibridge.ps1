# Deploy OmniBridge.dll to SteamVR driver folder
# Run this script as Administrator!

$source = "$PSScriptRoot\OmniBridge\publish\OmniBridge.dll"
$dest = "C:\Program Files (x86)\Steam\steamapps\common\SteamVR\drivers\treadmill\bin\win64\OmniBridge.dll"

if (-not (Test-Path $source)) {
    Write-Host "ERROR: Source not found: $source" -ForegroundColor Red
    Write-Host "Run 'dotnet publish -c Release -o publish' in OmniBridge folder first."
    exit 1
}

Write-Host "Copying OmniBridge.dll..." -ForegroundColor Cyan
Write-Host "  From: $source"
Write-Host "  To:   $dest"

try {
    Copy-Item $source -Destination $dest -Force -ErrorAction Stop
    
    $info = Get-Item $dest
    Write-Host ""
    Write-Host "SUCCESS!" -ForegroundColor Green
    Write-Host "  Size: $($info.Length) bytes"
    Write-Host "  Date: $($info.LastWriteTime)"
    Write-Host ""
    Write-Host "Log file after SteamVR start: $env:TEMP\OmniBridge.log"
}
catch {
    Write-Host ""
    Write-Host "ERROR: Copy failed!" -ForegroundColor Red
    Write-Host "Make sure:"
    Write-Host "  1. SteamVR is not running"
    Write-Host "  2. You're running as Administrator"
    Write-Host ""
    Write-Host "Error: $_"
}
