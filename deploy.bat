@echo off
echo ============================================================
echo   tabOS - Overforing og Kjøring på Allwinner A13 Nettbrett
echo ============================================================

where adb.exe >nul 2>nul
if %errorlevel% neq 0 (
    echo [FEIL] Fant ikke adb.exe i PATH!
    exit /b 1
)

if not exist tabos_arm (
    echo [!] tabos_arm ikke funnet. Bygger forst...
    call build.bat
    if %errorlevel% neq 0 exit /b %errorlevel%
)

echo [*] Stopper eventuell kjorende tabOS-prosess...
adb shell "pkill -9 tabos 2>/dev/null; true"

echo [*] Pusher tabos_arm til /data/local/tmp/tabos...
adb push tabos_arm /data/local/tmp/tabos
adb shell "chmod 755 /data/local/tmp/tabos"

echo [*] Starter tabOS pa Allwinner A13...
adb shell "/data/local/tmp/tabos"
