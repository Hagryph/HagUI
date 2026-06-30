# build.ps1 - build HagUI.dll and deploy it as a Mod Organizer 2 mod.
# MO2 mod folders mirror the game's Data/ root, so:
#   <mods>\HagUI\SKSE\Plugins\HagUI.dll
#   <mods>\HagUI\Interface\HagUI.swf   (once the SWF exists)
[CmdletBinding()]
param(
    [switch]$NoBuild,
    [string]$Mo2Mods = 'C:\Users\Yannis\AppData\Local\ModOrganizer\Skyrim Special Edition\mods'
)
$ErrorActionPreference = 'Stop'
$root  = $PSScriptRoot
$cmake = 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'

if (-not $NoBuild) {
    Set-Location $root
    $env:VCPKG_ROOT = 'C:\dev\vcpkg'
    Write-Host '== configure =='
    & $cmake --preset vs2022 | Select-Object -Last 2
    Write-Host '== build =='
    & $cmake --build "$root\build" --config Release | Select-Object -Last 4
    if ($LASTEXITCODE -ne 0) { throw "build failed (exit $LASTEXITCODE)" }
}

$dll = Get-ChildItem "$root\build" -Recurse -Filter HagUI.dll -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $dll) { throw 'HagUI.dll not found - build first (omit -NoBuild).' }

# --- create the MO2 mod folder architecture ---
$mod     = Join-Path $Mo2Mods 'HagUI'
$plugins = Join-Path $mod 'SKSE\Plugins'
$iface   = Join-Path $mod 'Interface'
New-Item -ItemType Directory -Force $plugins, $iface | Out-Null

Copy-Item $dll.FullName (Join-Path $plugins 'HagUI.dll') -Force
Write-Host "deployed HagUI.dll  -> $plugins"

$swf = Join-Path $root 'assets\HagUI.swf'
if (Test-Path $swf) {
    Copy-Item $swf (Join-Path $iface 'HagUI.swf') -Force
    Write-Host "deployed HagUI.swf  -> $iface"
} else {
    Write-Host '(HagUI.swf not built yet - skipped; drop it at assets\HagUI.swf)'
}

# Modified StartMenu.swf: vanilla main-menu movie + a "HagUI" entry above Credits whose
# click fires gfx.io.GameDelegate.call("OpenHagUI") -> our native handler. Loose file
# overrides the Interface BSA.
$startSwf = Join-Path $root 'assets\StartMenu.swf'
if (Test-Path $startSwf) {
    Copy-Item $startSwf (Join-Path $iface 'StartMenu.swf') -Force
    Write-Host "deployed StartMenu.swf -> $iface"
} else {
    Write-Host '(StartMenu.swf not present - skipped; the main-menu button needs it)'
}

# meta.ini so MO2 lists it cleanly
$meta = Join-Path $mod 'meta.ini'
if (-not (Test-Path $meta)) {
    @('[General]', 'gameName=Skyrim Special Edition', 'modid=0', 'version=0.1.0') |
        Set-Content $meta -Encoding UTF8
}

Write-Host "`nMO2 mod ready: $mod"
Get-ChildItem $mod -Recurse -File | ForEach-Object { '  ' + $_.FullName.Substring($mod.Length + 1) }
