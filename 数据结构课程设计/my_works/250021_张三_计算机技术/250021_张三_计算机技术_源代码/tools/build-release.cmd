@echo off
setlocal EnableExtensions
chcp 65001 >nul
call "%~dp0qt-env.cmd"
if errorlevel 1 goto failed

set "BUILD_DIR=%DNA_PROJECT_ROOT%\build-release"
if not exist "%BUILD_DIR%\CMakeCache.txt" (
    echo [ERROR] Release is not configured. Run configure-release.cmd first.
    goto failed
)
"%DNA_CMAKE%" --build "%BUILD_DIR%" --parallel
if errorlevel 1 goto failed
exit /b 0

:failed
echo.
echo [FAILED] Release build failed.
pause
exit /b 1
