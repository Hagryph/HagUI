# Build assets\HagUI.swf = hand-emitted stage + DoAction slot, then FFDec compiles
# our input-forwarding ActionScript (HagUI.as) into the frame-1 script.
# Requires JPEXS FFDec at C:\dev\ffdec (Java at C:\dev\jdk).
$ErrorActionPreference = 'Stop'
$assets = Join-Path (Split-Path $PSScriptRoot -Parent) 'assets'
$java   = 'C:\dev\jdk\jdk-21.0.11+10\bin\java.exe'
$jar    = 'C:\dev\ffdec\ffdec.jar'

# 1. emit the empty stage SWF with a frame-1 DoAction placeholder
& "$PSScriptRoot\make_stage_swf.ps1" | Out-Null

# 2. lay out the script folder FFDec expects, with our AS as the frame-1 script
$sf = Join-Path $assets '_scripts\scripts\frame_1'
New-Item -ItemType Directory -Force $sf | Out-Null
Copy-Item (Join-Path $assets 'HagUI.as') (Join-Path $sf 'DoAction.as') -Force

# 3. FFDec: compile the AS2 + inject it -> final HagUI.swf
& $java -jar $jar -importScript (Join-Path $assets 'HagUI_stage.swf') (Join-Path $assets 'HagUI.swf') (Join-Path $assets '_scripts') | Out-Null

# 4. tidy temporaries
Remove-Item (Join-Path $assets '_scripts'), (Join-Path $assets 'HagUI_stage.swf') -Recurse -Force -ErrorAction SilentlyContinue
$swf = Join-Path $assets 'HagUI.swf'
Write-Host ("built {0} ({1} bytes) with input ActionScript" -f $swf, (Get-Item $swf).Length)
