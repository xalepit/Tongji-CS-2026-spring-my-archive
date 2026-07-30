@echo off
setlocal EnableExtensions
chcp 65001 >nul
call "%~dp0qt-env.cmd"
if errorlevel 1 goto failed

set "BUILD_DIR=%DNA_PROJECT_ROOT%\build-release"
"%DNA_CMAKE%" -S "%DNA_PROJECT_ROOT%" -B "%BUILD_DIR%" -G Ninja ^
    "-DCMAKE_MAKE_PROGRAM=%DNA_NINJA%" ^
    "-DCMAKE_CXX_COMPILER=%DNA_GXX%" ^
    "-DCMAKE_PREFIX_PATH=%QT_PREFIX%" ^
    "-DCMAKE_BUILD_TYPE=Release"
if errorlevel 1 goto failed
exit /b 0

:failed
echo.
echo [FAILED] Release configuration failed.
pause
exit /b 1
