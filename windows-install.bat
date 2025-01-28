@echo off
setlocal enabledelayedexpansion

echo KhazarFetch Windows Installer
echo ===========================
echo.

:: Check for administrator privileges
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo Administrator rights are required for this operation.
    echo Please run this script as administrator.
    pause
    exit /b 1
)

echo Checking dependencies...

:: Check if chocolatey is installed
where choco >nul 2>nul
if %errorlevel% neq 0 (
    echo Installing Chocolatey package manager...
    @powershell -NoProfile -ExecutionPolicy Bypass -Command "iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))"
    :: Refresh environment variables
    call refreshenv
)

:: Check if g++ is installed
where g++ >nul 2>nul
if %errorlevel% neq 0 (
    echo Installing MinGW-w64 (C++ Compiler)...
    choco install mingw -y
    :: Refresh environment variables
    call refreshenv
)

:: Create program directory
echo Creating installation directory...
set "INSTALL_DIR=%ProgramFiles%\KhazarFetch"
if not exist "%INSTALL_DIR%" (
    mkdir "%INSTALL_DIR%"
)

:: Create a batch file to run khazarfetch
echo Creating launcher script...
set "LAUNCHER=%INSTALL_DIR%\khazarfetch.bat"
(
    echo @echo off
    echo "%INSTALL_DIR%\khazarfetch.exe" %%*
) > "%LAUNCHER%"

echo Building khazarfetch...
g++ src/khazarfetch-win.cpp -o "%INSTALL_DIR%\khazarfetch.exe" -lwbemuuid

:: Add to system PATH
echo Updating system PATH...
for /f "tokens=2*" %%a in ('reg query "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment" /v PATH') do set "current_path=%%b"
echo %current_path% | find /i "%INSTALL_DIR%" >nul || (
    setx /M PATH "%current_path%;%INSTALL_DIR%"
)

echo.
echo Installation completed successfully!
echo You can now use 'khazarfetch' from any terminal.
echo.
pause