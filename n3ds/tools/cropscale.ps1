# Crops a PNG and scales the crop up (nearest neighbour), for reading emulator screenshots.
# usage: powershell -File n3ds/tools/cropscale.ps1 in.png out.png x y w h scale
param([string]$in, [string]$out, [int]$x, [int]$y, [int]$w, [int]$h, [int]$scale = 2)
Add-Type -AssemblyName System.Drawing
$src = [System.Drawing.Image]::FromFile($in)
$dst = New-Object System.Drawing.Bitmap ($w * $scale), ($h * $scale)
$g = [System.Drawing.Graphics]::FromImage($dst)
$g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
$g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::Half
$g.DrawImage($src, (New-Object System.Drawing.Rectangle 0, 0, ($w * $scale), ($h * $scale)), $x, $y, $w, $h, [System.Drawing.GraphicsUnit]::Pixel)
$dst.Save($out, [System.Drawing.Imaging.ImageFormat]::Png)
$src.Dispose()
