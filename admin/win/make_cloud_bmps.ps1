# Disk Ra Grad: cloud pictures for the installer welcome page, one per screen scale (no stretching at runtime = sharp).
# ASCII only (Windows PowerShell 5.1). Source: admin/win/cloud-src.png (906x906, transparent; = disk-app/assets/logo.png).
# Usage: powershell -File make_cloud_bmps.ps1 -Src cloud-src.png -OutDir . [-BaseWidth 128]
# Output: cloud-100.bmp ... cloud-300.bmp, 24-bit, white background (page background of the installer is white).
param([string]$Src, [string]$OutDir, [int]$BaseWidth = 128)
Add-Type -AssemblyName System.Drawing
# note: PowerShell names are case-insensitive, so the picture must not be stored in $src (= the [string] parameter $Src)
$img = New-Object System.Drawing.Bitmap $Src
# bounds of the non-transparent part (alpha bytes via LockBits: GetPixel is too slow for 906x906)
$rect = New-Object System.Drawing.Rectangle(0, 0, $img.Width, $img.Height)
$data = $img.LockBits($rect, [System.Drawing.Imaging.ImageLockMode]::ReadOnly, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$bytes = New-Object byte[] ($data.Stride * $img.Height)
[System.Runtime.InteropServices.Marshal]::Copy($data.Scan0, $bytes, 0, $bytes.Length)
$stride = $data.Stride
$img.UnlockBits($data)
$minX = $img.Width; $minY = $img.Height; $maxX = -1; $maxY = -1
for ($y = 0; $y -lt $img.Height; $y += 1) {
    $row = $y * $stride
    for ($x = 0; $x -lt $img.Width; $x += 1) {
        if ($bytes[$row + $x * 4 + 3] -gt 8) {
            if ($x -lt $minX) { $minX = $x }; if ($x -gt $maxX) { $maxX = $x }
            if ($y -lt $minY) { $minY = $y }; if ($y -gt $maxY) { $maxY = $y }
        }
    }
}
$cw = $maxX - $minX + 1; $ch = $maxY - $minY + 1
"source crop: x=$minX y=$minY ${cw}x${ch}"
foreach ($pct in @(100, 125, 150, 175, 200, 250, 300)) {
    $w = [int][Math]::Round($BaseWidth * $pct / 100)
    $h = [int][Math]::Round($w * $ch / $cw)
    $bmp = New-Object System.Drawing.Bitmap $w, $h, ([System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.Clear([System.Drawing.Color]::White)
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
    $g.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
    $attr = New-Object System.Drawing.Imaging.ImageAttributes
    $attr.SetWrapMode([System.Drawing.Drawing2D.WrapMode]::TileFlipXY)
    $g.DrawImage($img,(New-Object System.Drawing.Rectangle(0, 0, $w, $h)), $minX, $minY, $cw, $ch, [System.Drawing.GraphicsUnit]::Pixel, $attr)
    $g.Dispose()
    $out = Join-Path $OutDir ("cloud-{0}.bmp" -f $pct)
    $bmp.Save($out, [System.Drawing.Imaging.ImageFormat]::Bmp)
    $bmp.Dispose()
    "saved $out ${w}x${h}"
}
$img.Dispose()
