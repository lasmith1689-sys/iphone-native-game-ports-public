@echo off
setlocal
cd /d "%~dp0"
PowerShell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0INSTALL_ON_QUEST3.ps1"
set "exitcode=%ERRORLEVEL%"
echo.
if not "%exitcode%"=="0" echo Installation did not finish. Read the instructions and try again.
echo Press any key to close this window.
pause >nul
exit /b %exitcode%
