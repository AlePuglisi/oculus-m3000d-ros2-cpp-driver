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
├── oculus_app/             # C++ app
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

If you want to add new features or understand the software structure, refer to [Software Strucure Notes](#software-structure-notes) section. 

## Quick Start

TODO: Describe how to easily launch the ROS2 Driver, visualize the sensor output and reconfigure it. 

## Extensive Usage Documentation

TODO: Describe more in details the driver additional features 

## Sonar Data Brief Description 

Preliminary notes on how the Oculus M3000d Sonar generate the acoustic images readings.

### Acoustic Image Formation Process 

Generic high level description of the logic behind acoustic images formation process. 

### Oculus Sonar Logic

Acoustic image formation process for the **oculus blueprint M3000d** series sonar. The same logic is used by previous version of the M series forward looking sonar, with some changes in the software implementation on the sdk. 
<br/>(Probably other FLS sensors uses similar workflows)






## Software Structure Notes

Software structure of the low level C++ Sonar Driver and the overlay ROS2 Interface. 

### Low Level Driver (oculus_driver)

C++ library to interface with the Sonar Hardware.

Expose classes to connect, configure and read the Sonar data. 

### ROS2 Driver

ROS2 Jazzy (C++) packages to interface with the Oculus M3000d in a ROS2 infrastructure. 

Handling sonar initial configuration and dynamic reconfigure, UDP connection, sensor read, raw sensor read processing, and polar image conversion to cartesian.

Other features will be integrated according to research task requrements. 

### oculus_interface

[This package](/oculus_interfaces/) contains the custom ROS2 sonar messages interfaces. <br/>
It allow to exchange in the ROS network equivalent informatioon to the ones used by the propritary software.

These ROS2 messages reproduce the official oculus sdk messages data structures.

<details close>
<summary>Implementation Details</summary>
  
</details>

### oculus_ros2

[This package](/oculus_ros2/) handles the interface with the sonar hardware and the data processing. 


#### [``conversions.hpp``](/oculus_ros2/include/oculus_ros2/conversions.hpp)

This utils code implements conversions from oculus messages to ROS2 messages.

When the ``oculus_sonar_node`` receives the sonar messages from the low level driver instance, it rely on these methods to ROSify them and publish on the ROS network.  

<details close>
<summary>Implementation Details</summary>
  
</details>


#### [``oculus_sonar_node``](/oculus_ros2/src/oculus_sonar_node.cpp)

This Node handles the interface with the sonar hardware thorugh an instance of the low level sonar driver class. 

It has the follwoing **main tasks**: 
- Connect to the sonar via UDP 
- Sett the initial configuration of the sensor (frequency mode, range, gain, etc..)
- ROSify sonar measurements and broadcast to the nework 
- Handle reconfiguartion requests

The sensor configuration is handled by this node ROS parameters, initialized by a config file: [oculus_sonar.yaml](/oculus_ros2/cfg/oculus_sonar.yaml)

The node implements the parameter reconfiguration callback, sending **dynamic reconfiguration** request to the sonar. <br/>
NOTE: This part is still under development and adjustments.

<details close>
<summary>Implementation Details</summary>
  
</details>

#### [``sonar_viewer_node``](/oculus_ros2/src/sonar_viewer_node.cpp)

This Node process the raw sonar data (as bearings-bins matrix) to create a polar and cartesian acoustic image.

It allows to perform physical based pre-processing on the raw data. <br/>
In the proprietary visualizatiion software (Oculus ViewPoint), as in other sonars, some software logic is used to correct the image stream in a pleasant view. What you see usually is a processed version of the raw acoustic echo. Moreover, also at the retrieval level in the transducers the gain is adapted during the image formation, each bins can be adjusted with a different gain value. <br/>

The process of changing the gain applied on each bins (range) based on echo intensity and other factors is called **Adaptive Gain Control** (AGC). This range-gains are included in the sonar readings. 


Due to the resons above, in this node it is possible to decide if retrieving the original raw echo, by "removing" the broadcasted gains of the AGC process. Afterward, custom pre-processing can be applied on retrieved the raw data. <br/>
The sonar_viewer_node provides a **transmission loss correction**, which apply a time-propagation and water absorption loss gain to the acoustic data. 

The above features are handled by ROS parameters (allwoing also online reconfiguration). <br/>Set ``remove_agc`` to true to override the applied transducer range-gains abd retrieve the raw data. <br/>
Set ``apply_tvg`` to true to apply the custom physicial correction gain. 

**WARNING**<br/>
Be carefully on how you combine the two pre-processing features. <br/>
By applying the TVG without removing the AGC (remove_agc = false, apply_tvg = true) the sensor readings will be satured by applying correction gain twice (both AGC and TL).  


<details close>
<summary>Implementation Details</summary>
  
</details>

#### [``sonar_point_cloud_node``](/oculus_ros2/src/sonar_point_cloud_node.cpp)

This Node publishes a point cloud computed from the cartesian sonar image. <br/>
The points are selcted as the ones with pixel intensity (echo intensity) higher than a specified user threshold. 
Due to the acoustic image formation model, this naive solution form a point cloud in sonar zero elevation plane (where the sonar frustum is projected). 

<details close>
<summary>Implementation Details</summary>
  
</details>

#### [``sonar_state_estimation_node``](/oculus_ros2/src/sonar_state_estimation_node.cpp)

This Node exploits the proprioceptive sonar information (depth and orientation) coming from a depth sensor and an AHRS.

After the sensor readings, the pose (depth position + orientation) is published as a ROS TransForm frame (TF). <br/>
This allow a visualization of the sonar frame in rviz (a 3D textured model and a sensor URDF description is defined in the package)


<details close>
<summary>Implementation Details</summary>
  
</details>

#### [``sonar_saver_node``](/oculus_ros2/src/sonar_saver_node.cpp)

This Node should be runned to save **all** the cartesian and polar acoustic images inside of a specified output directory. 

It uses the ROS cv_bridge utils to convert ROS images in the OpenCV format, ready to be saved in the desired image compression format.  


<details close>
<summary>Implementation Details</summary>
  
</details>

## Problems and Possible Improvements