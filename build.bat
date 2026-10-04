@echo off
setlocal enabledelayedexpansion

echo ============================================================
echo   tabOS - Allwinner A13 Bare-Metal / Linux Cross-Compiler
echo ============================================================

:: Finn Zig i PATH eller standard WinGet-sti
where zig.exe >nul 2>nul
if %errorlevel% equ 0 (
    set ZIG=zig.exe
) else (
    set ZIG=%LOCALAPPDATA%\Microsoft\WinGet\Packages\zig.zig_Microsoft.Winget.Source_8wekyb3d8bbwe\zig-x86_64-windows-0.17.0\zig.exe
)

if not exist "!ZIG!" (
    echo [FEIL] Fant ikke zig.exe! Installer Zig via: winget install zig.zig
    exit /b 1
)

echo [*] Bruker Zig kompilator: "!ZIG!"
echo [*] Maalarmarkitektur: ARMv7-A (arm-linux-musleabihf, Cortex-A8, sun5i)
echo [*] Kompilerer tabOS kjerne og alle applikasjoner...

!ZIG! cc -target arm-linux-musleabihf -O3 -Iinclude ^
    src/main.c ^
    src/os.c ^
    src/display.c ^
    src/input.c ^
    src/sensor.c ^
    src/audio.c ^
    src/power.c ^
    src/synth.c ^
    src/apps/app_launcher.c ^
    src/apps/app_bbs.c ^
    src/apps/app_snake.c ^
    src/apps/app_keyboard.c ^
    src/apps/app_touchtest.c ^
    src/apps/app_colortest.c ^
    src/apps/app_settings.c ^
    src/apps/app_synth.c ^
    -lm -o tabos_arm

if %errorlevel% neq 0 (
    echo [FEIL] Kompilering feilet!
    exit /b %errorlevel%
)

echo [*] Bygging fullfort! Fil generert: tabos_arm
echo ============================================================
echo   Kjor 'deploy.bat' for a overfore og starte pa nettbrettet!
echo ============================================================
