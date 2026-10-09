# Saves a PNG of a process's main window (used to screenshot the emulator).
# usage: powershell -File n3ds/tools/winshot.ps1 <process name> <out.png>
param([string]$name = "azahar", [string]$out = "shot.png")
Add-Type -AssemblyName System.Drawing
Add-Type @"
using System;
using System.Runtime.InteropServices;
public class Win {
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint flags);
    [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
}
"@
[Win]::SetProcessDPIAware() | Out-Null
$p = Get-Process $name -ErrorAction Stop | Where-Object { $_.MainWindowHandle -ne 0 } | Select-Object -First 1
$r = New-Object Win+RECT
[Win]::GetWindowRect($p.MainWindowHandle, [ref]$r) | Out-Null
$w = $r.R - $r.L; $h = $r.B - $r.T
$bmp = New-Object System.Drawing.Bitmap $w, $h
$g = [System.Drawing.Graphics]::FromImage($bmp)
$dc = $g.GetHdc()
# 2 = PW_RENDERFULLCONTENT, needed for GPU-rendered windows.
[Win]::PrintWindow($p.MainWindowHandle, $dc, 2) | Out-Null
$g.ReleaseHdc($dc)
$bmp.Save($out, [System.Drawing.Imaging.ImageFormat]::Png)
"$w x $h -> $out"
