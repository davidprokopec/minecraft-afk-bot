@echo off
REM Build script for mc-afker project (Windows)

setlocal enabledelayedexpansion

REM Default values
if "%BUILD_TYPE%"=="" set BUILD_TYPE=Release
if "%BUILD_DIR%"=="" set BUILD_DIR=build

echo mc-afker Build Script
echo Build type: %BUILD_TYPE%
echo Build directory: %BUILD_DIR%
echo.

REM Create build directory
echo Creating build directory...
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

REM Configure CMake
echo Configuring CMake...
cd "%BUILD_DIR%"
cmake .. -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
if errorlevel 1 (
    echo CMake configuration failed!
    exit /b 1
)

REM Build
echo Building project...
cmake --build . --config %BUILD_TYPE%
if errorlevel 1 (
    echo Build failed!
    exit /b 1
)

echo.
echo Build completed successfully!
echo Binary location: %BUILD_DIR%\bin\mc-afker.exe

cd ..

