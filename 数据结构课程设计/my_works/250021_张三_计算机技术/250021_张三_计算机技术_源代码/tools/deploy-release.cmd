@echo off
setlocal EnableExtensions
chcp 65001 >nul
call "%~dp0qt-env.cmd"
if errorlevel 1 goto failed

set "BUILD_EXE=%DNA_PROJECT_ROOT%\build-release\DNA_SearchEngine.exe"
set "RELEASE_ROOT=%DNA_PROJECT_ROOT%\release"
set "RELEASE_DIR=%RELEASE_ROOT%\DNA_SearchEngine"
set "STAGING_DIR=%RELEASE_ROOT%\.DNA_SearchEngine-staging"
set "BACKUP_DIR=%RELEASE_ROOT%\.DNA_SearchEngine-backup"

echo.
echo === DNA Search Engine Release Deployment ===
echo Source: "%BUILD_EXE%"
echo Target: "%RELEASE_DIR%"
echo.

if not exist "%BUILD_EXE%" (
    echo [ERROR] Release executable was not found.
    echo Run configure-release.cmd and build-release.cmd first.
    goto failed
)

if not exist "%RELEASE_ROOT%" mkdir "%RELEASE_ROOT%"
if exist "%STAGING_DIR%" rmdir /s /q "%STAGING_DIR%"
if exist "%STAGING_DIR%" (
    echo [ERROR] Could not clean the staging directory.
    goto failed
)
if exist "%BACKUP_DIR%" rmdir /s /q "%BACKUP_DIR%"
if exist "%BACKUP_DIR%" (
    echo [ERROR] Could not clean the previous backup directory.
    goto failed
)

mkdir "%STAGING_DIR%"
if errorlevel 1 goto failed
copy /y "%BUILD_EXE%" "%STAGING_DIR%\DNA_SearchEngine.exe" >nul
if errorlevel 1 goto failed

set "PATH=%DNA_MINGW_BIN%;%QT_PREFIX%\bin;%PATH%"
echo Running windeployqt...
"%DNA_WINDEPLOYQT%" --release --compiler-runtime --no-translations ^
    "%STAGING_DIR%\DNA_SearchEngine.exe"
if errorlevel 1 (
    echo [ERROR] windeployqt failed.
    goto failed
)

for %%F in (libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll) do (
    if not exist "%STAGING_DIR%\%%F" (
        copy /y "%DNA_MINGW_BIN%\%%F" "%STAGING_DIR%\%%F" >nul
        if errorlevel 1 (
            echo [ERROR] Could not copy %%F.
            goto failed
        )
    )
)

for %%F in (
    "DNA_SearchEngine.exe"
    "Qt6Core.dll"
    "Qt6Gui.dll"
    "Qt6Widgets.dll"
    "libgcc_s_seh-1.dll"
    "libstdc++-6.dll"
    "libwinpthread-1.dll"
    "platforms\qwindows.dll"
) do (
    if not exist "%STAGING_DIR%\%%~F" (
        echo [ERROR] Deployment validation failed: missing %%~F
        goto failed
    )
)

if exist "%RELEASE_DIR%" (
    move "%RELEASE_DIR%" "%BACKUP_DIR%" >nul
    if errorlevel 1 (
        echo [ERROR] Could not replace the current release.
        echo Close every running DNA_SearchEngine.exe and try again.
        goto failed
    )
)

move "%STAGING_DIR%" "%RELEASE_DIR%" >nul
if errorlevel 1 (
    echo [ERROR] Could not activate the new release. Rolling back...
    if not exist "%RELEASE_DIR%" if exist "%BACKUP_DIR%" (
        move "%BACKUP_DIR%" "%RELEASE_DIR%" >nul
    )
    goto failed
)

if exist "%BACKUP_DIR%" rmdir /s /q "%BACKUP_DIR%"
echo.
echo [SUCCESS] Deployment completed.
echo "%RELEASE_DIR%\DNA_SearchEngine.exe"
echo.
pause
exit /b 0

:failed
if defined STAGING_DIR if exist "%STAGING_DIR%" rmdir /s /q "%STAGING_DIR%"
echo.
echo [FAILED] Deployment did not modify a complete existing release.
echo Keep this window open and inspect the error above.
echo.
pause
exit /b 1
