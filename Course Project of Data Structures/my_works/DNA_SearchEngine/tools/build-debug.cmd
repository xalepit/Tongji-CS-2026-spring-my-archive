@echo off
setlocal EnableExtensions
chcp 65001 >nul
call "%~dp0qt-env.cmd"
if errorlevel 1 goto failed

set "BUILD_DIR=%DNA_PROJECT_ROOT%\build-debug"
if not exist "%BUILD_DIR%\CMakeCache.txt" (
    echo [ERROR] Debug is not configured. Run configure-debug.cmd first.
    goto failed
)
"%DNA_CMAKE%" --build "%BUILD_DIR%" --parallel
if errorlevel 1 goto failed
exit /b 0

:failed
echo.
echo [FAILED] Debug build failed.
pause
exit /b 1
