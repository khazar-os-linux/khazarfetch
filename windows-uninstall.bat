@echo off
echo KhazarFetch Uninstaller
echo =====================
echo.

:: Check for administrator privileges
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo Administrator rights are required for this operation.
    echo Please run this script as administrator.
    pause
    exit /b 1
)

:: Remove program files
echo Removing program files...
set "INSTALL_DIR=%ProgramFiles%\KhazarFetch"
if exist "%INSTALL_DIR%" (
    rd /s /q "%INSTALL_DIR%"
)

:: Remove from PATH
echo Updating system PATH...
for /f "tokens=2*" %%a in ('reg query "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment" /v PATH') do set "current_path=%%b"
set "new_path=!current_path:%INSTALL_DIR%;=!"
setx /M PATH "%new_path%"

echo.
echo KhazarFetch has been successfully uninstalled.
echo.
pause