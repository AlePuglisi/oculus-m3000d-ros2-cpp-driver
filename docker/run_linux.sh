#!/bin/bash

# Script to run Oculus ROS2 Docker on Linux/Raspberry Pi OS

set -e

echo "===================================="
echo "Oculus Sonar ROS2 Docker - Linux"
echo "===================================="
echo ""

# Check if Docker is installed
if ! command -v docker &> /dev/null; then
    echo "ERROR: Docker is not installed!"
    echo "Please install Docker and try again."
    exit 1
fi

# Check if Docker is running
if ! docker info &> /dev/null; then
    echo "ERROR: Docker daemon is not running!"
    echo "Please start Docker and try again."
    exit 1
fi

# Allow X server connections
echo "Configuring X11 permissions..."
xhost +local:docker || true

# Set DISPLAY if not already set
if [ -z "$DISPLAY" ]; then
    export DISPLAY=:0
fi

echo "Using DISPLAY=$DISPLAY"
echo ""

# Build the image if needed
if ! docker images | grep -q "oculus_ros2"; then
    echo "Building Docker image..."
    docker compose build
fi

# Function to clean up on exit
cleanup() {
    echo ""
    echo "Cleaning up..."
    xhost -local:docker || true
}

trap cleanup EXIT

# Run the container
echo "Starting Docker container..."
docker compose up -d

# Attach to the container
echo ""
echo "Attaching to container... (Ctrl+D or 'exit' to detach)"
echo ""
docker exec -it oculus_sonar_driver bash

# Optionally stop the container on exit
read -p "Stop the container? (y/N): " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    docker compose down
fi