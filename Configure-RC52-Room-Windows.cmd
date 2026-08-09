@echo off
setlocal
cd /d "%~dp0"

where py >nul 2>nul
if errorlevel 1 (
  echo Python 3 was not found. Install it from https://www.python.org/downloads/windows/
  pause
  exit /b 1
)

if not exist ".neonpocket-venv\Scripts\python.exe" (
  echo Preparing the one-time setup helper...
  py -3 -m venv .neonpocket-venv || goto :failed
)

".neonpocket-venv\Scripts\python.exe" -m pip install --disable-pip-version-check -q pyserial || goto :failed
".neonpocket-venv\Scripts\python.exe" scripts\configure_rc52_room.py
set "RESULT=%ERRORLEVEL%"
echo.
pause
exit /b %RESULT%

:failed
echo.
echo Setup helper preparation failed. Check your internet connection and Python installation.
pause
exit /b 1
