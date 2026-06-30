# Mirror the built HagUI.dll (+ optional .swf) into the live Skyrim install.
$ErrorActionPreference = 'Stop'
$game = 'C:\Program Files (x86)\Steam\steamapps\common\Skyrim Special Edition'
$plugins = Join-Path $game 'Data\SKSE\Plugins'
New-Item -ItemType Directory -Force $plugins | Out-Null

$dll = Get-ChildItem "$PSScriptRoot\build" -Recurse -Filter HagUI.dll -ErrorAction SilentlyContinue |
    Select-Object -First 1
if (-not $dll) { throw "HagUI.dll not found - build first: cmake --build build --config Release" }
Copy-Item $dll.FullName (Join-Path $plugins 'HagUI.dll') -Force
Write-Host "Deployed $($dll.FullName) -> $plugins\HagUI.dll"

# Ship the Scaleform movie too, once we have one.
$swf = Join-Path $PSScriptRoot 'assets\HagUI.swf'
if (Test-Path $swf) {
    $iface = Join-Path $game 'Data\Interface'
    New-Item -ItemType Directory -Force $iface | Out-Null
    Copy-Item $swf (Join-Path $iface 'HagUI.swf') -Force
    Write-Host "Deployed HagUI.swf -> $iface"
}
