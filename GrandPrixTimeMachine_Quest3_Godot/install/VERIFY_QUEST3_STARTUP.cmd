@echo off
setlocal
cd /d "%~dp0"
PowerShell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0VERIFY_QUEST3_STARTUP.ps1"
set "exitcode=%ERRORLEVEL%"
echo.
if not "%exitcode%"=="0" echo Startup was not confirmed. See the local report.
echo Press any key to close this window.
pause >nul
exit /b %exitcode%
