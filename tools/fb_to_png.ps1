param(
    [string]$RawFile = "fb0.raw",
    [string]$OutputFile = "docs\screenshots\live_screen.png",
    [int]$Width = 800,
    [int]$Height = 480
)

Add-Type -AssemblyName System.Drawing

$bytes = [System.IO.File]::ReadAllBytes($RawFile)
$bmp = New-Object System.Drawing.Bitmap($Width, $Height, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$rect = New-Object System.Drawing.Rectangle(0, 0, $Width, $Height)
$bmpData = $bmp.LockBits($rect, [System.Drawing.Imaging.ImageLockMode]::WriteOnly, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
[System.Runtime.InteropServices.Marshal]::Copy($bytes, 0, $bmpData.Scan0, $Width * $Height * 4)
$bmp.UnlockBits($bmpData)

$outDir = [System.IO.Path]::GetDirectoryName($OutputFile)
if (-not [string]::IsNullOrEmpty($outDir) -and -not (Test-Path $outDir)) {
    New-Item -ItemType Directory -Force -Path $outDir | Out-Null
}

$bmp.Save($OutputFile, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()
Write-Host "Lagret skjermbilde til $OutputFile"
