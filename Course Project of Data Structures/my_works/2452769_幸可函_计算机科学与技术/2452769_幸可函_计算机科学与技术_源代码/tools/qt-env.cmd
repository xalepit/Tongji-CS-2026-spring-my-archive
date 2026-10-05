@echo off
setlocal EnableExtensions

for %%I in ("%~dp0..") do set "DNA_PROJECT_ROOT=%%~fI"

if not defined QT_PREFIX set "QT_PREFIX=D:\Qt\6.11.1\mingw_64"
if not defined DNA_MINGW_BIN set "DNA_MINGW_BIN=D:\Qt\Tools\mingw1310_64\bin"
if not defined DNA_CMAKE set "DNA_CMAKE=D:\Qt\Tools\CMake_64\bin\cmake.exe"
if not defined DNA_NINJA set "DNA_NINJA=D:\Qt\Tools\Ninja\ninja.exe"

set "DNA_WINDEPLOYQT=%QT_PREFIX%\bin\windeployqt.exe"
set "DNA_GXX=%DNA_MINGW_BIN%\g++.exe"

if not exist "%QT_PREFIX%\bin\qmake.exe" (
    echo [ERROR] Qt was not found at "%QT_PREFIX%".
    echo Set QT_PREFIX to the Qt mingw_64 directory and try again.
    exit /b 1
)
if not exist "%DNA_GXX%" (
    echo [ERROR] MinGW compiler was not found at "%DNA_GXX%".
    echo Set DNA_MINGW_BIN to the matching MinGW bin directory.
    exit /b 1
)
if not exist "%DNA_CMAKE%" (
    echo [ERROR] CMake was not found at "%DNA_CMAKE%".
    exit /b 1
)
if not exist "%DNA_NINJA%" (
    echo [ERROR] Ninja was not found at "%DNA_NINJA%".
    exit /b 1
)
if not exist "%DNA_WINDEPLOYQT%" (
    echo [ERROR] windeployqt was not found at "%DNA_WINDEPLOYQT%".
    exit /b 1
)

endlocal & (
    set "DNA_PROJECT_ROOT=%DNA_PROJECT_ROOT%"
    set "QT_PREFIX=%QT_PREFIX%"
    set "DNA_MINGW_BIN=%DNA_MINGW_BIN%"
    set "DNA_CMAKE=%DNA_CMAKE%"
    set "DNA_NINJA=%DNA_NINJA%"
    set "DNA_WINDEPLOYQT=%DNA_WINDEPLOYQT%"
    set "DNA_GXX=%DNA_GXX%"
)
exit /b 0
