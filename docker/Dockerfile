# Dockerfile for Oculus M3000d Blueprint Sonar ROS2 Driver
# Supports Ubuntu 24.04, ROS2 Jazzy, with GUI support for RViz

# Base stage with ROS2 Jazzy
FROM ros:jazzy AS base

# Set environment variables
ENV DEBIAN_FRONTEND=noninteractive
ENV ROS_DISTRO=jazzy
ENV LANG=C.UTF-8
ENV LC_ALL=C.UTF-8

# Install system dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    python3-pip \
    libboost-system-dev \
    libboost-thread-dev \
    libopencv-dev \
    libeigen3-dev \
    net-tools \
    iputils-ping \
    vim \
    ros-jazzy-rviz2 \
    ros-jazzy-rqt \
    ros-jazzy-rqt-reconfigure \
    mesa-utils \
    x11-apps \
    && rm -rf /var/lib/apt/lists/*

# Build stage for oculus_driver (C++ low-level driver)
FROM base AS driver-builder

WORKDIR /tmp/oculus_driver_build

# Copy the local oculus_driver_lib from the project
COPY ../oculus_driver_lib .

# Build and install oculus_driver
RUN mkdir -p build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release \
          -DCMAKE_INSTALL_PREFIX=/usr/local \
          .. && \
    make -j$(nproc) && \
    make install

# ROS2 workspace build stage
FROM base AS ros2-builder

# Copy installed oculus_driver from previous stage
COPY --from=driver-builder /usr/local/include/oculus_driver /usr/local/include/oculus_driver
COPY --from=driver-builder /usr/local/lib/liboculus_driver.so* /usr/local/lib/
COPY --from=driver-builder /usr/local/lib/cmake/oculus_driver /usr/local/lib/cmake/oculus_driver

# Update library cache
RUN ldconfig

# Create ROS2 workspace
WORKDIR /ros2_ws

# Copy your ROS2 packages from the local project structure
# The OCULUS_ROS2 directory contains both oculus_interfaces and oculus_ros2 packages
COPY ../OCULUS_ROS2 src/oculus_ros2

# Install ROS2 dependencies
RUN apt-get update && \
    rosdep update && \
    rosdep install --from-paths src --ignore-src -r -y && \
    rm -rf /var/lib/apt/lists/*

# Build ROS2 workspace
RUN . /opt/ros/${ROS_DISTRO}/setup.sh && \
    colcon build \
    --cmake-args -DCMAKE_BUILD_TYPE=Release \
    --symlink-install

# Final runtime stage
FROM base AS runtime

# Install runtime dependencies only
RUN apt-get update && apt-get install -y \
    libboost-system1.83.0 \
    libboost-thread1.83.0 \
    libopencv-dev \
    libeigen3-dev \
    net-tools \
    iputils-ping \
    ros-${ROS_DISTRO}-rviz2 \
    ros-${ROS_DISTRO}-rqt \
    ros-${ROS_DISTRO}-rqt-reconfigure \
    x11-apps \
    mesa-utils \
    && rm -rf /var/lib/apt/lists/*

# Copy oculus_driver library
COPY --from=driver-builder /usr/local/include/oculus_driver /usr/local/include/oculus_driver
COPY --from=driver-builder /usr/local/lib/liboculus_driver.so* /usr/local/lib/
COPY --from=driver-builder /usr/local/lib/cmake/oculus_driver /usr/local/lib/cmake/oculus_driver

# Copy built ROS2 workspace
COPY --from=ros2-builder /ros2_ws /ros2_ws

# Update library cache
RUN ldconfig

# Set up entrypoint
WORKDIR /ros2_ws

# Create entrypoint script
RUN echo '#!/bin/bash\n\
set -e\n\
\n\
# Source ROS2\n\
source /opt/ros/${ROS_DISTRO}/setup.bash\n\
source /ros2_ws/install/setup.bash\n\
\n\
# Execute command\n\
exec "$@"' > /ros2_entrypoint.sh && \
    chmod +x /ros2_entrypoint.sh

ENTRYPOINT ["/ros2_entrypoint.sh"]
CMD ["bash"]