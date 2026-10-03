param(
    [string]$OutputFile = "docs\screenshots\screenshot.png"
)

adb -s 20080411413fc082 shell "cat /dev/graphics/fb0 > /data/local/tmp/fb_dump.raw"
adb -s 20080411413fc082 pull /data/local/tmp/fb_dump.raw fb_dump.raw

& powershell -ExecutionPolicy Bypass -File tools\fb_to_png.ps1 -RawFile "fb_dump.raw" -OutputFile $OutputFile

Remove-Item -Force fb_dump.raw
Write-Host "Ferdig: $OutputFile"
