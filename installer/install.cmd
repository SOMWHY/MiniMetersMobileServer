@echo off
REM Double-click entry point for the PowerShell installer: the script needs an
REM elevated console, and double-clicking a .ps1 opens Notepad instead of running it.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Install-MiniMetersMobile.ps1" %*
if errorlevel 1 (
    echo.
    echo Installation failed. See INSTALL.md for the manual steps.
    pause
)
