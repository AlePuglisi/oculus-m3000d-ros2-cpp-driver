# Oculus M3000d Sonar C++ Interface Documentation

C++ Driver for the Blueprint subsea Oculus M3000d Sonar. 

The ROS2 Jazzy driver is available in the [``ros2-jazzy`` branch of this repo](https://github.com/AlePuglisi/oculus-ros2-cpp-driver/tree/ros2-jazzy)

###  Software "Development": Updating Extisting Drivers

This package is adapted and extended from the M1200d [ROS2 Driver](https://github.com/godardma/oculus_ros2/tree/main) by ENSTA Bretagne Robotics, and the underlying low level Driver [Sonar Interface](https://github.com/ENSTABretagneRobotics/oculus_driver)

The mentioned M1200d software has been updated for the M3000d using the blueye proprietary **oculus_sdk** <br/>
(shared by the technical support team).

## Requirements

This C++ Application was developed and tested using:
* Ubuntu 24.04 LTS
* CMake 3.28.3

## Documentation Contents

- [Repository Structure](#repository-structure)
- [Installation](#installation)
- [Quick Start](#quick-start)
- [Extensive Usage Documentation](#extensive-usage-documentation)
- [Sonar Data Brief Description](#sonar-data-brief-description)
- [Software Structure Notes](#software-structure-notes)
- [Problems and Possible Improvements](#problems-and-possible-improvements)

## Repository Structure

This repository contains all the code necessary to build and use the blueprint subsea oculus M3000d sonar driver. Moreover, it include the original proprietary sdk (the one used in the official blueprint ViewPointv2) as reference code. Note in fact that the sdk code is not used during the building process. 

### Folder Structure

```
oculus-ros2-cpp-driver/
├── docker/                  # Docker configuration files
├── oculus_driver_lib/       # C++ driver library
├── oculus_app/              # C++ app
├── reference_oculus_sdk/    # Reference sdk code 
├── LICENSE
└── README.md
```


## Installation 

### 0) Setting Up the Workspace

1. **Clone or Copy this repo (cpp branch)**

    ```
    cd src
    git clone --branch cpp https://github.com/AlePuglisi/oculus-ros2-cpp-driver.git
    ```

2. **Move in the driver directory for the next steps**

    ```
    cd oculus-ros2-cpp-driver
    ```

### 1) Install the oculus_driver 

1. **Install dependencies**

    ```
    sudo apt-get install -y libboost-system-dev libboost-thread-dev
    ```

2. **Compile the oculus_driver library**

    This library follows a standard CMake compilation procedure.


    From the git cloned directory of this driver, move to the lib directory 

    ```
    cd oculus_driver_lib
    ```

    Create a build directory :

    ```
    mkdir build && cd build
    ```

3. **Compile the oculus_dirver library**

    Generate your build system with CMake, compile and install :

    ```
    cmake ..
    make -j4
    ```

4. **Install the driver** 

    ```
    sudo make -j4 install
    ```

### 2) Build the oculus_app

If all the steps above compiled without errors, you currently have installed the oculus_driver library. 

1. **Move to the C++ project directory and make build directory**

```
cd oculus_app && mkdir build
```

2. **Build the project with your favourite g++ builder**

```
cd build
cmake ..
make -j4
```

3. **The extecutable is ready**

```
./oculus_app
```

### Integrating New Features and Understading the Driver

If you want to add new features or understand the software structure, <br/> refer to the [Software Strucure Notes](#software-structure-notes) section. 

## Quick Start

TODO: Describe how to easily launch the ROS2 Driver, visualize the sensor output and reconfigure it. 

## Extensive Usage Documentation

TODO: Describe more in details the driver additional features 

## Sonar Data Brief Description 
TODO: Copy the final version of this documentation part from jazzy branch as soon as finished. 

### Acoustic Image Formation Process 
TODO: Copy the final version of this documentation part from jazzy branch as soon as finished. 

### Oculus Sonar Logic
TODO: Copy the final version of this documentation part from jazzy branch as soon as finished. 

## Software Structure Notes

Software structure of the low level C++ Sonar Driver and the simple C++ Interface application.  

### Low Level Driver (oculus_driver)


C++ library to interface with the Sonar Hardware.

Expose classes to connect, configure and read the Sonar data.


### Oculus App

C++ packages to interface with the Oculus M3000d sensor. 

Handling sonar initial configuration and dynamic reconfigure, UDP connection, sensor read, raw sensor read processing, and polar image conversion to cartesian.

Other features will be integrated according to research task requrements. 

### [``OculusApp.hpp``](/oculus_app/include/oculus_app/OculusApp.hpp)

This handles the interface with the sonar hardware. 

<details close>
<summary>Implementation Details</summary>
  
</details>

### [``OculusViewer.hpp``](/oculus_app/include/oculus_app/OculusViewer.hpp)

This class handles the received ping messages to visualize it in an OpenCV window. 

<details close>
<summary>Implementation Details</summary>
  
</details>

### [``sonar_utils.hpp``](/oculus_app/include/oculus_app/sonar_utils.hpp)

This code provides some utils on the sonar data. 

<details close>
<summary>Implementation Details</summary>
  
</details>

### [``Logger.hpp``](/oculus_app/include/oculus_app/Logger.hpp)

This class provides some basic logging utilities. 

<details close>
<summary>Implementation Details</summary>
  
</details>

## Problems and Possible Improvements