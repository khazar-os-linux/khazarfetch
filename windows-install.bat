@echo off
setlocal enabledelayedexpansion

echo Checking dependencies...

:: Check if chocolatey is installed
where choco >nul 2>nul
if %errorlevel% neq 0 (
    echo Installing Chocolatey package manager...
    @powershell -NoProfile -ExecutionPolicy Bypass -Command "iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1'))"
    :: Refresh environment variables
    refreshenv
)

:: Check if g++ is installed
where g++ >nul 2>nul
if %errorlevel% neq 0 (
    echo Installing MinGW-w64 (C++ Compiler)...
    choco install mingw -y
    :: Refresh environment variables
    refreshenv
)

echo Creating installation directory...
if not exist "%USERPROFILE%\AppData\Local\Programs\khazarfetch" (
    mkdir "%USERPROFILE%\AppData\Local\Programs\khazarfetch"
)

echo Building khazarfetch...
g++ src/main.cpp -o khazarfetch.exe

echo Installing khazarfetch...
move khazarfetch.exe "%USERPROFILE%\AppData\Local\Programs\khazarfetch\"

:: Add to PATH if not already added
echo Updating PATH...
set "PATH_TO_ADD=%USERPROFILE%\AppData\Local\Programs\khazarfetch"
echo %PATH% | find /i "%PATH_TO_ADD%" >nul || (
    setx PATH "%PATH%;%PATH_TO_ADD%"
)

echo Installation completed successfully!
echo You can now use khazarfetch from any terminal.
pause