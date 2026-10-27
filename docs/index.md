# Oculus M3000d Sonar ROS2 Driver

ROS2 (Jazzy) driver for the Blueprint Subsea Oculus M3000d dual-frequency multibeam sonar.
This driver should work with any sonar of the M series.

A C++ (without ROS) sonar interface is under development in the
[`cpp` branch of this repo](https://github.com/AlePuglisi/oculus-m3000d-ros2-cpp-driver/tree/cpp).

The purpose of this driver is to retrieve raw sonar data with custom code during sensor deployment,
either in a ROS2 perception-control stack, or in a custom C++ architecture for the robotic system.
It is not a substitute for the vendor application
[Oculus ViewPoint V2](https://www.blueprintsubsea.com/oculus/support.php).

```{figure} assets/images/Oculus_Driver_Overview.png
:width: 100%
:alt: ROS2 communication architecture

Fig. 1: ROS2 communication architecture. The `oculus_sonar` node instantiates a driver client to
establish TCP/UDP communication with the hardware. The `sonar_viewer` node converts the received echo
to a cartesian (fan) image and a polar image, while the `sonar_point_cloud` node publishes the planar
point cloud by thresholding high-intensity echoes. `sonar_state_estimation` is a prototype that uses the
sensor AHRS and pressure sensor to estimate the sensor pose, but it is currently not reliable.
```

## Software "Development": Updating Existing Drivers

This package is adapted and extended from the M1200d
[ROS2 driver](https://github.com/godardma/oculus_ros2/tree/main) by ENSTA Bretagne Robotics, and the
underlying low-level driver [Sonar Interface](https://github.com/ENSTABretagneRobotics/oculus_driver).

The M1200d software has been updated for the M3000d using the up-to-date
[Blueprint Subsea](https://www.blueprintsubsea.com/) proprietary
[oculus_sdk](./../reference_oculus_sdk/)
(kindly shared by the technical support team).

Some useful PDF documents to develop the sonar software, use the sensor and interpret the data are
collected in the
[assets](./assets/) folder of the repository.

## Requirements

This ROS2 metapackage (with the low-level sonar driver) was developed, tested, and built using:

* Ubuntu 24.04 LTS
* ROS2 Jazzy
* CMake 3.28.3

A Dockerfile with instructions to build the image and run it from any machine is planned.
(The current [docker](./../docker/) setup has not been tested.)

## Repository Structure

This repository contains all the code necessary to build and use the Blueprint Subsea Oculus M3000d
sonar driver. It also includes the original proprietary SDK (the one used in the official Blueprint
ViewPoint V2) as reference code. Note that the SDK code is **not** used during the build process.

```text
oculus-m3000d-ros2-cpp-driver/
├── docker/                  # Docker configuration files (work in progress...)
├── assets/                  # Documentation images and files
├── oculus_driver_lib/       # C++ driver library
├── oculus_ros2/             # ROS2 packages
│   ├── oculus_interfaces/   # Messages / services
│   ├── oculus_ros2/         # Main driver package
│   └── Data/                # Optional data directory
├── reference_oculus_sdk/    # Reference SDK code
├── LICENSE
└── README.md
```

```{toctree}
:maxdepth: 2
:caption: Contents

installation
quickstart_sensor
quickstart_driver
sonar_data
m3000d_in_depth
software_structure
problems
```
