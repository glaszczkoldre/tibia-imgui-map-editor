@echo off
setlocal enabledelayedexpansion

REM ============================================================
REM  Tibia Map Editor Ninja Build Script
REM  Builds x64 Release with Ninja and generates compile_commands.json
REM  All detailed output is logged to build_ninja.log
REM ============================================================

set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR:~0,-1%"
set "SOURCE_DIR=%PROJECT_ROOT%\ImguiMapEditor"
set "BUILD_DIR=%PROJECT_ROOT%\build-ninja"
set "LOG_FILE=%PROJECT_ROOT%\build_ninja.log"

REM Start fresh log
echo ============================================================ > "%LOG_FILE%"
echo   Tibia Map Editor - Build Log                           >> "%LOG_FILE%"
echo   Started: %DATE% %TIME%                                  >> "%LOG_FILE%"
echo ============================================================ >> "%LOG_FILE%"
echo. >> "%LOG_FILE%"

echo.
echo ========================================================
echo   Tibia Map Editor Ninja Build (x64 Release)
echo   Log: build_ninja.log
echo ========================================================
echo.

echo [1/6] Checking for Ninja...
ninja --version >nul 2>&1
if !ERRORLEVEL! neq 0 (
    echo ERROR: Ninja was not found.
    exit /b 1
)
echo OK - Ninja found.

echo [2/6] Finding Visual C++ toolchain...
set "VW=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VW%" (
    echo ERROR: vswhere.exe not found.
    exit /b 1
)

set "VSP="
for /f "usebackq tokens=*" %%i in (`"%VW%" -latest -products * -property installationPath`) do (
    set "VSP=%%i"
)

if "%VSP%"=="" (
    echo ERROR: Visual Studio installation not found.
    exit /b 1
)

echo Found VS at: %VSP%
echo Initializing x64 environment...

set "VV=%VSP%\VC\Auxiliary\Build\vcvarsall.bat"
if not exist "%VV%" (
    echo ERROR: vcvarsall.bat not found.
    exit /b 1
)

call "%VV%" x64 >nul
if !ERRORLEVEL! neq 0 (
    echo ERROR: Failed to initialize x64 environment.
    exit /b 1
)
echo OK - Environment initialized.

echo [3/6] Locating vcpkg...
set "VKD="
if defined VCPKG_ROOT (
    if exist "%VCPKG_ROOT%\.vcpkg-root" set "VKD=%VCPKG_ROOT%"
)
if not defined VKD (
    for %%p in ("c:\vcpkg" "c:\src\vcpkg" "%USERPROFILE%\vcpkg") do (
        if exist "%%~p\.vcpkg-root" set "VKD=%%~p"
    )
)
if not defined VKD (
    echo ERROR: vcpkg not found. Set VCPKG_ROOT environment variable.
    exit /b 1
)
echo OK - vcpkg found at: %VKD%

echo [4/6] Configuring CMake (Ninja)...
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

cmake -G Ninja ^
      -S "%SOURCE_DIR%" ^
      -B "%BUILD_DIR%" ^
      -DCMAKE_BUILD_TYPE=Release ^
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ^
      -DCMAKE_TOOLCHAIN_FILE="%VKD%\scripts\buildsystems\vcpkg.cmake" ^
      -DVCPKG_TARGET_TRIPLET=x64-windows ^
      >> "%LOG_FILE%" 2>&1

if !ERRORLEVEL! neq 0 (
    echo ERROR: CMake configuration failed.
    echo See %LOG_FILE% for details.
    echo --- Last 30 lines of log: ---
    powershell -NoProfile -Command "Get-Content '%LOG_FILE%' | Select-Object -Last 30"
    exit /b 1
)
echo OK - Configuration complete.

echo [5/6] Building TibiaMapEditor...
cmake --build "%BUILD_DIR%" --target TibiaMapEditor --parallel >> "%LOG_FILE%" 2>&1
if !ERRORLEVEL! neq 0 (
    echo.
    echo ========================================================
    echo   BUILD FAILED
    echo ========================================================
    echo.
    echo See %LOG_FILE% for full build output.
    echo --- Last 40 lines of log: ---
    powershell -NoProfile -Command "Get-Content '%LOG_FILE%' | Select-Object -Last 40"
    exit /b 1
)
echo OK - Build complete.

echo [6/6] Updating compile_commands.json...
if exist "%BUILD_DIR%\compile_commands.json" (
    copy /Y "%BUILD_DIR%\compile_commands.json" "%PROJECT_ROOT%\compile_commands.json" >nul
    echo OK - compile_commands.json copied to root.
) else (
    echo WARNING: compile_commands.json not found.
)

echo.
echo ========================================================
echo   BUILD SUCCESSFUL
echo ========================================================
echo.
echo   Project:   %SOURCE_DIR%
echo   Output:    %BUILD_DIR%\TibiaMapEditor.exe
echo   Linter:    %PROJECT_ROOT%\compile_commands.json
echo   Log:       %LOG_FILE%
echo.

endlocal
exit /b 0
