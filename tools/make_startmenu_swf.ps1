# make_startmenu_swf.ps1 - regenerate assets\StartMenu.swf (the modified main-menu movie).
#
# We DON'T commit Bethesda's StartMenu.swf. Instead we rebuild it on demand from the
# player's own game install: extract the vanilla movie from Skyrim - Interface.bsa,
# apply three small ActionScript edits, recompile with JPEXS FFDec. The edits add a
# "HagUI" entry above Credits whose click fires gfx.io.GameDelegate.call("OpenHagUI"),
# which our SKSE plugin (HagMenu) registers on MainMenu::RegisterFuncs.
#
# Requires: JDK at C:\dev\jdk, FFDec at C:\dev\ffdec, and the base game installed.
[CmdletBinding()]
param(
    [string]$SkyrimData = 'C:\Program Files (x86)\Steam\steamapps\common\Skyrim Special Edition\Data',
    [string]$Java       = 'C:\dev\jdk\jdk-21.0.11+10\bin\java.exe',
    [string]$Ffdec      = 'C:\dev\ffdec\ffdec.jar'
)
$ErrorActionPreference = 'Stop'
$assets = Join-Path (Split-Path $PSScriptRoot -Parent) 'assets'
$work   = Join-Path $env:TEMP ('hag_startmenu_' + $PID)
New-Item -ItemType Directory -Force $assets, $work | Out-Null

# --- 1. extract interface\startmenu.swf from the (uncompressed v105) Interface BSA ---
function Extract-BsaFile([string]$bsaPath, [string]$wantName, [string]$outPath) {
    $fs = [System.IO.File]::OpenRead($bsaPath); $br = New-Object System.IO.BinaryReader($fs)
    try {
        $br.ReadBytes(4) | Out-Null                       # 'BSA\0'
        $ver = $br.ReadUInt32()
        if ($ver -ne 105) { throw "unexpected BSA version $ver (expected 105)" }
        $br.ReadUInt32() | Out-Null                       # header offset
        $br.ReadUInt32() | Out-Null                       # archive flags
        $folderCount = $br.ReadUInt32(); $br.ReadUInt32() | Out-Null
        $br.ReadUInt32() | Out-Null; $totFileName = $br.ReadUInt32()
        $br.ReadUInt16() | Out-Null; $br.ReadUInt16() | Out-Null
        $counts = @()                                     # folder record = 24 bytes (hash8,count4,pad4,offset8)
        for ($i = 0; $i -lt $folderCount; $i++) {
            $br.ReadUInt64() | Out-Null; $c = $br.ReadUInt32(); $br.ReadUInt32() | Out-Null; $br.ReadUInt64() | Out-Null
            $counts += $c
        }
        $files = @()                                      # file record blocks (sequential): name bzstring + 16-byte records
        foreach ($cnt in $counts) {
            $nl = $br.ReadByte(); $br.ReadBytes($nl) | Out-Null
            for ($j = 0; $j -lt $cnt; $j++) { $br.ReadUInt64() | Out-Null; $sz = $br.ReadUInt32(); $of = $br.ReadUInt32(); $files += [pscustomobject]@{ size = $sz; offset = $of } }
        }
        $names = [System.Text.Encoding]::ASCII.GetString($br.ReadBytes($totFileName)).TrimEnd([char]0).Split([char]0)
        for ($k = 0; $k -lt $files.Count; $k++) { $files[$k] | Add-Member name $names[$k] }
        $t = $files | Where-Object { $_.name -ieq $wantName } | Select-Object -First 1
        if (-not $t) { throw "$wantName not found in $bsaPath" }
        $comp = ($t.size -band 0x40000000) -ne 0; $real = $t.size -band 0x3FFFFFFF
        $fs.Position = $t.offset
        if ($comp) {
            $br.ReadUInt32() | Out-Null; $br.ReadBytes(2) | Out-Null
            $ds = New-Object System.IO.Compression.DeflateStream($fs, [System.IO.Compression.CompressionMode]::Decompress)
            $ms = New-Object System.IO.MemoryStream; $ds.CopyTo($ms); $bytes = $ms.ToArray()
        } else { $bytes = $br.ReadBytes($real) }
        [System.IO.File]::WriteAllBytes($outPath, $bytes)
    } finally { $br.Close(); $fs.Close() }
}

$bsa     = Join-Path $SkyrimData 'Skyrim - Interface.bsa'
$vanilla = Join-Path $work 'StartMenu_vanilla.swf'
Extract-BsaFile $bsa 'startmenu.swf' $vanilla
Write-Host ("extracted vanilla StartMenu.swf ({0} bytes)" -f (Get-Item $vanilla).Length)

# --- 2. export the StartMenu class, apply the three edits (idempotent) ---
$exp = Join-Path $work 'as'
& $Java -jar $Ffdec -export script $exp $vanilla | Out-Null
$asPath = Join-Path $exp 'scripts\__Packages\StartMenu.as'
$as = Get-Content $asPath -Raw
if ($as -notmatch 'HAGUI_INDEX') {
    $as = $as -replace '(\r?\n)(\s*static var PS5_DATA_TRANSFER_INDEX = 11;)', "`$1`$2`$1   static var HAGUI_INDEX = 90;"
    $as = $as -replace '(\s*)(this\.MainList\.entryList\.push\(\{text:"\$CREDITS")',
        "`$1this.MainList.entryList.push({text:`"HagUI`",index:StartMenu.HAGUI_INDEX,disabled:false,showIcon:false});`$1`$2"
    $as = $as -replace '(\s*)(case StartMenu\.CREDITS_INDEX:)',
        "`$1case StartMenu.HAGUI_INDEX:`$1   gfx.io.GameDelegate.call(`"OpenHagUI`",[]);`$1   gfx.io.GameDelegate.call(`"PlaySound`",[`"UIMenuOK`"]);`$1   return;`$1`$2"
    Set-Content $asPath $as -Encoding UTF8 -NoNewline
    Write-Host 'applied HagUI ActionScript edits'
} else { Write-Host 'edits already present (skipped)' }

# --- 3. recompile only the edited class back into the movie ---
$imp = Join-Path $work 'import\scripts\__Packages'
New-Item -ItemType Directory -Force $imp | Out-Null
Copy-Item $asPath (Join-Path $imp 'StartMenu.as') -Force
$out = Join-Path $assets 'StartMenu.swf'
& $Java -jar $Ffdec -importScript $vanilla $out (Join-Path $work 'import') | Out-Null
Remove-Item $work -Recurse -Force -ErrorAction SilentlyContinue
Write-Host ("built {0} ({1} bytes)" -f $out, (Get-Item $out).Length)
