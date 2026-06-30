# make_stage_swf.ps1 - emit a minimal, valid 1-frame "stage" SWF for HagUI.
# No external tooling: HagUI draws all content at runtime via GFxValue, so the
# movie only needs to be an empty stage Scaleform can instantiate. Output:
#   <repo>\assets\HagUI.swf   (uncompressed FWS v8, 1280x720, near-black bg)
$ErrorActionPreference = 'Stop'
$out = Join-Path $PSScriptRoot '..\assets\HagUI.swf'
New-Item -ItemType Directory -Force (Split-Path $out) | Out-Null

# --- bit writer for the FrameSize RECT ---
$bits = New-Object System.Collections.Generic.List[int]
function AddBits([int]$value, [int]$n) { for ($i = $n - 1; $i -ge 0; $i--) { $bits.Add(($value -shr $i) -band 1) } }
AddBits 16 5        # nbits per field
AddBits 0 16        # Xmin
AddBits 25600 16    # Xmax = 1280px * 20 twips
AddBits 0 16        # Ymin
AddBits 14400 16    # Ymax = 720px * 20 twips
while ($bits.Count % 8 -ne 0) { $bits.Add(0) }
$rect = New-Object System.Collections.Generic.List[byte]
for ($i = 0; $i -lt $bits.Count; $i += 8) {
    $b = 0; for ($j = 0; $j -lt 8; $j++) { $b = ($b -shl 1) -bor $bits[$i + $j] }; $rect.Add([byte]$b)
}

# --- body: RECT + frame rate + frame count + tags ---
$body = New-Object System.Collections.Generic.List[byte]
$body.AddRange($rect)
$body.Add([byte]0); $body.Add([byte]30)        # FrameRate 8.8 LE -> 30 fps
$body.Add([byte]1); $body.Add([byte]0)         # FrameCount = 1
$bg = (9 -shl 6) -bor 3                          # SetBackgroundColor tag (id 9, len 3)
$body.Add([byte]($bg -band 0xFF)); $body.Add([byte](($bg -shr 8) -band 0xFF))
$body.Add([byte]0x0A); $body.Add([byte]0x0A); $body.Add([byte]0x0C)  # RGB near-black (#0A0A0C)
$body.Add([byte]0x40); $body.Add([byte]0x00)   # ShowFrame (id 1, len 0)
$body.Add([byte]0x00); $body.Add([byte]0x00)   # End (id 0, len 0)

# --- header: 'FWS' + version + file length ---
$fileLen = 8 + $body.Count
$swf = New-Object System.Collections.Generic.List[byte]
$swf.AddRange([byte[]][char[]]'FWS')
$swf.Add([byte]8)                               # SWF version 8 (Flash 8 / AVM1, Scaleform GFx 3.x)
$swf.Add([byte]($fileLen -band 0xFF)); $swf.Add([byte](($fileLen -shr 8) -band 0xFF))
$swf.Add([byte](($fileLen -shr 16) -band 0xFF)); $swf.Add([byte](($fileLen -shr 24) -band 0xFF))
$swf.AddRange($body)

[System.IO.File]::WriteAllBytes((Resolve-Path -LiteralPath (Split-Path $out)).Path + '\' + (Split-Path $out -Leaf), $swf.ToArray())
Write-Host ("wrote {0} ({1} bytes)" -f $out, $swf.Count)
Write-Host ("hex: " + (($swf.ToArray() | ForEach-Object { $_.ToString('x2') }) -join ' '))
