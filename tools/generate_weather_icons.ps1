param(
    [int]$Size = 50
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$projectRoot = Split-Path -Parent $PSScriptRoot
$assetsDir = Join-Path $projectRoot "assets/weather_icons"
$headerOut = Join-Path $projectRoot "include/weather_icons.h"
$sourceOut = Join-Path $projectRoot "src/weather_icons.c"

$codes = @(
    "01d", "01n",
    "02d", "02n",
    "03d",
    "04d",
    "09d",
    "10d", "10n",
    "11d",
    "13d",
    "50d"
)

Add-Type -AssemblyName System.Drawing
[System.Drawing.Bitmap]$null = $null

function Convert-ToRgb565Bytes {
    param(
        [byte]$R,
        [byte]$G,
        [byte]$B
    )

    $r5 = [int]($R * 31 / 255)
    $g6 = [int]($G * 63 / 255)
    $b5 = [int]($B * 31 / 255)
    $rgb565 = ($r5 -shl 11) -bor ($g6 -shl 5) -bor $b5

    # High byte first because LV_COLOR_16_SWAP = 1 in lv_conf.h
    return @([byte](($rgb565 -shr 8) -band 0xFF), [byte]($rgb565 -band 0xFF))
}

$transparent = [System.Drawing.Color]::FromArgb(0, 0, 0, 0)

$headerLines = [System.Collections.Generic.List[string]]::new()
$headerLines.Add("#pragma once")
$headerLines.Add("")
$headerLines.Add("#include <lvgl.h>")
$headerLines.Add("")
foreach ($code in $codes) {
    $headerLines.Add("extern const lv_img_dsc_t weather_icon_${code};")
}
[System.IO.File]::WriteAllLines($headerOut, $headerLines)

$lines = [System.Collections.Generic.List[string]]::new()
$lines.Add('#include "weather_icons.h"')
$lines.Add("")

foreach ($code in $codes) {
    $iconPath = Join-Path $assetsDir "${code}.png"
    if (-not (Test-Path $iconPath)) {
        throw "Missing icon file: $iconPath"
    }

    $src = [System.Drawing.Image]::FromFile($iconPath)
    $bmp = New-Object System.Drawing.Bitmap($Size, $Size, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $gfx = [System.Drawing.Graphics]::FromImage($bmp)

    $gfx.Clear($transparent)
    $gfx.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
    $gfx.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $gfx.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
    $gfx.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $gfx.DrawImage($src, 0, 0, $Size, $Size)

    $src.Dispose()
    $gfx.Dispose()

    $bytes = [System.Collections.Generic.List[byte]]::new()
    for ($y = 0; $y -lt $Size; $y++) {
        for ($x = 0; $x -lt $Size; $x++) {
            $c = $bmp.GetPixel($x, $y)
            $rgbBytes = Convert-ToRgb565Bytes -R $c.R -G $c.G -B $c.B
            [void]$bytes.Add($rgbBytes[0])
            [void]$bytes.Add($rgbBytes[1])
            [void]$bytes.Add($c.A)
        }
    }

    $bmp.Dispose()

    $lines.Add("static const uint8_t weather_icon_${code}_map[] = {")
    for ($i = 0; $i -lt $bytes.Count; $i += 16) {
        $max = [Math]::Min($i + 15, $bytes.Count - 1)
        $chunk = for ($j = $i; $j -le $max; $j++) { "0x{0:X2}" -f $bytes[$j] }
        $lines.Add("    " + ($chunk -join ", ") + ", ")
    }
    $lines.Add("};")
    $lines.Add("")
    $lines.Add("const lv_img_dsc_t weather_icon_${code} = {")
    $lines.Add("    .header.always_zero = 0,")
    $lines.Add("    .header.w = $Size,")
    $lines.Add("    .header.h = $Size,")
    $lines.Add("    .data_size = sizeof(weather_icon_${code}_map),")
    $lines.Add("    .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,")
    $lines.Add("    .data = weather_icon_${code}_map,")
    $lines.Add("};")
    $lines.Add("")
}

[System.IO.File]::WriteAllLines($sourceOut, $lines)
Write-Output "Generated $sourceOut and $headerOut ($Size x $Size)"
