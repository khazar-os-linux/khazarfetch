@echo off
echo KhazarFetch 2.0 Uninstaller
echo ==========================
echo.

:: Check for administrator privileges
net session >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Administrator rights are required for this operation.
    echo Please run this script as administrator.
    pause
    exit /b 1
)

:: Remove the program
echo Removing KhazarFetch...
if exist "%PROGRAMFILES%\KhazarFetch" (
    rd /s /q "%PROGRAMFILES%\KhazarFetch"
)

:: Remove from PATH
echo Updating PATH...
for /f "tokens=2*" %%a in ('reg query "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment" /v PATH') do set "CURRENT_PATH=%%b"
set "NEW_PATH=!CURRENT_PATH:%PROGRAMFILES%\KhazarFetch;=!"
setx PATH "%NEW_PATH%" /M

echo.
echo KhazarFetch has been successfully uninstalled.
echo.
pause