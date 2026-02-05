@echo off
REM Script to run Oculus ROS2 Docker on Windows with VcXsrv

echo ====================================
echo Oculus Sonar ROS2 Docker - Windows
echo ====================================
echo.

REM Check if Docker is running
docker info >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo ERROR: Docker is not running!
    echo Please start Docker Desktop and try again.
    pause
    exit /b 1
)

REM Get Windows host IP for WSL2
for /f "tokens=2 delims=:" %%a in ('ipconfig ^| findstr /C:"WSL"') do set WSLIP=%%a
set WSLIP=%WSLIP: =%

REM If WSL IP not found, use default
if "%WSLIP%"=="" (
    echo Using localhost for DISPLAY
    set DISPLAY_IP=host.docker.internal
) else (
    echo Found WSL IP: %WSLIP%
    set DISPLAY_IP=%WSLIP%
)

echo.
echo ====================================
echo IMPORTANT: X Server Setup
echo ====================================
echo Please ensure VcXsrv is running with:
echo   - Display number: 0
echo   - Disable access control: CHECKED
echo   - Extra settings: -ac -nowgl
echo.
echo Download VcXsrv from: https://sourceforge.net/projects/vcxsrv/
echo ====================================
echo.

pause

REM Set DISPLAY environment variable
set DISPLAY=%DISPLAY_IP%:0.0

echo Starting Docker container with DISPLAY=%DISPLAY%
echo.

REM Build the image if it doesn't exist
docker images | findstr oculus_ros2 >nul
if %ERRORLEVEL% neq 0 (
    echo Building Docker image...
    docker-compose build
)

REM Run the container
docker-compose up -d

REM Attach to the container
echo.
echo Attaching to container... (Ctrl+C to detach)
echo.
docker exec -it oculus_sonar_driver bash

pause