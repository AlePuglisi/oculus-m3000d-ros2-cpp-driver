# Oculus M3000d Sonar ROS2 Driver Documentation

ROS2 (Jazzy) Driver for the Blueprint Subsea Oculus M3000d Dual-Frequency Multibeam Sonar. 

A C++ (Without ROS) Sonar interface is under development in the [``cpp`` branch of this repo](https://github.com/AlePuglisi/oculus-ros2-cpp-driver/tree/cpp)

The purpose of this driver is to be able to retrieve raw sonar data with custom code during sensor deployment, either in a ROS2 perception-control stack, or in a custom C++ architecture for the robotic system.<br/>
It is not a substitute of the vendor application [Oculus ViewPoint V2](https://www.blueprintsubsea.com/oculus/support.php).

<p>
    <img width=1000 src=docs/images/Oculus_Driver_Overview.png alt>
</p>
<p>
    <em>Fig1: ROS2 Communication architecture. The oculus_sonar node instantiate a driver client to  establish TCP/UDP communication with the hardware.
    The sonar_viewer processes the received processed echo to cartesian (fan) and polar image, while the sonar_point_cloud node publish the planar point cloud by thresholding high intensity echoes. sonar_state_estimation is a prototype using the sensor AHRS and pressure sensor to estimate sensor pose, but currently it not reliable.</em>
</p>

###  Software "Development": Updating Extisting Drivers

This package is adapted and extended from the M1200d [ROS2 Driver](https://github.com/godardma/oculus_ros2/tree/main) by ENSTA Bretagne Robotics, and the underlying low level Driver [Sonar Interface](https://github.com/ENSTABretagneRobotics/oculus_driver).<br/>
The mentioned M1200d software has been updated for the M3000d using the up-to-date Blueye proprietary [oculus_sdk](./reference_oculus_sdk/) (shared by the technical support team).<br/>

Some useful PDF documents to develop the sonar software, use the sensor and interpret the data are collected in the [docs](docs/) folder of this repo.

## Requirements

This ROS2 metapackage (with the low-level sonar driver) was developed, tested, and built using:
* Ubuntu 24.04 LTS
* ROS2 jazzy
* CMake 3.28.3

I'm planning to provide a Dockerfile and the relative instructions to build the image and run it with Docker from any machine.
(The current [docker](./docker/) set up has not been tested)

## Documentation Contents

- [Repository Structure](#repository-structure)
- [Installation](#installation)
- [Quick Start - Sensor Set Up](#quick-start-sensor-set-up)
- [Quick Start - Using the Driver](#quick-start-using-the-driver)
- [Sonar Data Brief Description](#sonar-data-brief-description)
- [Oculus M300d in Depth](#oculus-m3000d-in-depth)
- [Software Structure Notes](#software-structure-notes)
- [Problems and Possible Improvements](#problems-and-possible-improvements)

## Repository Structure

This repository contains all the code necessary to build and use the blueprint subsea oculus M3000d sonar driver. Moreover, it include the original proprietary sdk (the one used in the official blueprint ViewPointv2) as reference code. Note, in fact, that the sdk code is not used during the building process. 

### Folder Structure

```
oculus-ros2-cpp-driver/
├── docker/                  # Docker configuration files (work in progress...)
├── docs/                    # Documentation images and files 
├── oculus_driver_lib/       # C++ driver library
├── oculus_ros2/             # ROS2 packages
│   ├── oculus_interfaces/   # Message/service 
│   ├── oculus_ros2/         # Main driver package
│   └── Data/                # Optional data directory
├── reference_oculus_sdk/    # Reference sdk code 
├── LICENSE
└── README.md
```


## Installation (Tested on Ubuntu 24.04 + ROS2 Jazzy) 

### 0) Setting Up the Workspace

1. **Install and set up [ROS2 jazzy](http://wiki.ros.org/ROS/Installation)**

    <details> 
    <summary> Install and Set Up ROS2 Jazzy </summary>
    
    - 1.1) **Install** ROS Jazzy and source 
    Refer to [ros documentation](https://docs.ros.org/en/jazzy/Installation/Ubuntu-Install-Debs.html) for the step-by-step installation tutorial. <br/>

        Anytime you open a new terminal, you should source your underlay ros environment with:
        ```
        # Terminal
        # Replace ".bash" with your shell if you're not using bash
        # Possible values are: setup.bash, setup.sh, setup.zsh
        source /opt/ros/jazzy/setup.bash
        ```
        To avoid doing it every time, add that command to your ``bashrc``, opening it with a text editor of your choice:
        ```
        # Terminal (use your preferred text editor tool)
        gedit ~/.bashrc
        ```
        And add the line ``source /opt/ros/jazzy/setup.bash`` at the end.

    - 1.2) If not automatically installed, **install ``colcon``**, the build tool for ROS2: 
        ```
        # Terminal
        sudo apt install python3-colcon-common-extensions
        ```

    - 1.3)** Create and build a **ros2 workspace** to position this project packages:<br/>
    In your preferred directory
        ```
        # Terminal
        mkdir sonar_ws
        cd sonar_ws
        mkdir src
        colcon build
        ```

    - **1.4)** **Source** your ros2 workspace and the colcon argmícomplete environment:<br/>
    As done for the ros underlay, you need to source the overlay setup.bash when you build it, again you can add that to the bashrc
        ```
        # Terminal
        gedit ~/.bashrc
        ```
        Add the line ``source ~/sonar_ws/install/setup.bash`` <br/>
        and ``source /usr/share/colcon_argcomplete/hook/colcon-argcomplete.bash`` to complete colocn commands by pressing Tab.

    - **1.5)** **Install** Additional Packages
        To launch this project packages, you may need ROS2 packages not installed with ros-jazzy-desktop. <br/>
        I will update this list during implementation:

        - ``xacro``
        - ``image_transport``
        - ...

        ```
        # Terminal
        sudo apt install ros-${ROS_DISTRO}-xacro ros-${ROS_DISTRO}-image-transport
        ```
    </details>
    <br/>

2. **Create the ros2 workspace**

    ```
    mkdir sonar_ws && cd sonar_ws
    ```
3. **Create src directory and build**
    ```
    mkdir src
    ```
    ```
    colcon build 
    ```

4. **Clone or Copy this repo in /src**
    ```
    cd src
    git clone https://github.com/AlePuglisi/oculus-ros2-cpp-driver.git
    ```

5. **Move in the driver dircetory for the next steps**
    ```
    cd oculus-ros2-cpp-driver
    ```

### 1) Install the oculus_driver 

1. **Install dependencies**

    ```
    sudo apt-get install -y libboost-system-dev libboost-thread-dev
    ```

2. **Set up the build for the oculus_driver library**

    This library follows a standard CMake compilation procedure.


    From the git cloned directory of this driver, move to the lib directory 

    ```
    cd oculus_driver_lib
    ```

    Create a build directory :

    ```
    mkdir build && cd build
    ```

3. **Build the oculus_dirver library**

    Generate your build system with CMake, compile and install :

    ```
    cmake ..
    make -j4
    ```

    <details close>
    <summary>With More Explicit Building Commands</summary>

    ```
        cmake -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX=/usr/local \
            .. && \
        make -j$(nproc) && \
        make install
    ```
    </details>

4. **Install the driver** 

    ```
    sudo make -j4 install
    ```

    This will install the driver at the system level (in /usr/local/lib), not locally in the workspace. 
    If you want the driver to be installed only inside of the workspace you should modify the `oculus_driver_lib` [CMakeLists.txt](./oculus_driver_lib/CMakeLists.txt). Or use an Explicit Building Commands where `DCMAKE_INSTALL_PREFIX` indicate a different install path. 
    Notice that if the `oculus_driver` library is installed elsewhere, also `oculus_ros2` CMakeLists may need some changes to properly find the library.

### 2) Build the oculus_ros2 package

If all the steps above compiled without errors, you currently have a ros2 workspace ready to be built and the oculus_driver library properly installed. 

The ROS2 packages (oculus_interface + oculus_ros2) building is the usual one.

Be sure to be in the workspace root directory (e.g. ~/sonar_ws) and run 

```
colcon build 
```


### Integrating New Features and Understading the Driver

If you want to add new features or understand the software structure, refer to [Software Strucure Notes](#software-structure-notes) section. 

## Quick Start Sensor Set Up 

### 1) Sonar-Computer Ethernet Connection 

Connecting the Sonar to the computer (Ethernet connection) <br/>


> [!WARNING]  
> Never power on the Sonar when it is not underwater. 
> Connect the plug or switch on the power source only when the sonar is in the water. 
> The electronics are built to work when surrounded by water (cooling system), it heats up very quickly and it can broke if this is not respected. 


<p>
    <img width=600 src=docs/images/Sensor_Set_Up.png alt>
</p>
<p>
    <em>Fig2: Oculus Sonar connection scheme. The Computer (6) here will ru the ROS2 driver (or the C++ one)</em>
</p>


### 2) Configure Wired Ethernet Connection

In your computer configure the ethernet connection in IPv4 with  <br/>
**IPv4 Method**: `Manual` <br/>
**Addresses**: <br/>
Address = x1.x2.x3.x4+1 (where x1.x2.x3.x4 is the sonar ip) <br/>
Netmask = 255.255.255.0 <br/>

The sonar ip x1.x2.x3.x4 can be found easily by opening the [Oculus ViewPoint](https://www.blueprintsubsea.com/oculus/support.php) app and searching for the connected devices. The address is found by UDP port, and the IP will be visible in the listed devices. 


## Quick Start Using the Driver 

To start the oculus M3000d driver, different launch files are available for basic functionalities or more "advanced" features. 


> [!IMPORTANT]  
> You cannot start the ros2 driver while the Oculus ViewPoint is running. 
> If you try to launch any of the ROS2 driver scripts with the ViewPoint visualization running, it will not work. 


> [!IMPORTANT]  
> Be sure that you don't have any firewall active on your device. This can cause connection problem both in the ViewPoint and with the ROS2 driver. You can disable it in a terminal by running the following command: `sudo ufw disable`
    


### 0) Define the Initial Configuration

If you are running one of the launch file, you can set up the initial node's parameter configuration in a `.yaml` file. <br/>
Navigate to [/OCULUS_ROS2/oculus_ros2/cfg](OCULUS_ROS2/oculus_ros2/cfg/) directory, where you can find the configuration file used for any node. 

Setting up the driver and the processing node in advance is not crucial, since dynamic parameter reconfiguration is handled by the nodes, as described next.

### 1) Start the Driver + optional Sonar Processing Nodes

- **Sonar Driver** <br/>

    To run only the driver node, run the basic driver node (handling the ROS2 interface with the low level sonar driver library). 

    ```
    ros2 run oculus_ros2 oculus_sonar_node
    ```
    
    <details close>
    <summary>Exposed Topics</summary>
        If the driver run without any topic or namespace remapping (as the command above), it will expose: <br/><br/>
        \status : high-level status information about sensor state, device id, ip, and other information remapped in other topics for easy access. <br/>
        \ping : encode the overall ping return in a readable way, including processed sound received, timestamp, etc. <br/>
        \oculus_ping : encode the overall ping return. <br/>
        \temperature : Internal electronics temperature. Used to enter in protection mode when temperature is too high.<br/>
        \pressure : depth pressure to which the sensor is subjected. USed to retrieve depth for sound speed computation.<br/><br/>
        Additionally, the reconfiguration ROS2 node parameters are exposed to change the sonar configuration (frequency mode, gain, etc)<br/>
        Refer to the section below "Reconfiguring the sonar" for further details on this. 
    </details>
    <br/>

- **Sonar Driver + Image Formation**<br/>

    ```
    ros2 launch oculus_ros2 oculus_ros2_basic.launch.py
    ```

    This will run the `oculus_sonar_node` and `sonar_viewer_node`.

- **Sonar Driver + Image Formation + Point Cloud + rviz2**<br/>
   
    ```
    ros2 launch oculus_ros2 oculus_ros2_full.launch.py
    ```
### 2) Reconfiguring the Sonar and the Processing Nodes 

Most of the configurations of the Sonar Driver and the Processing Nodes (such as the viewer and the point cloud publisher) are handled by ROS2 node's parameters. <br/>
This allows a reconfiguration during sensor usage, without restarting the process (using the parameter reconfiguration callback).

It is possible to reconfigure the paramters using the CLI tools exposed by ROS2, but it is more confortable to use the rqt Dynamic reconfigure Plugin. Follow the steps below to easily start it: 

launch the ros2 `rqt` as
```
rqt
```

Open the parameter reconfiguration window and navigating to <br/> `> Plugins > Configuration > Dynamic Reconfigure`

From there select the `oculus_sonar` driver node, and you will see all the reconfigurable sonar parameters (frequency mode, gain, range, etc).
In the same way, if you want to change the polar-to-cartesian conversion reolution or other aspect of the viewer, select the `sonar_viewer` node. This holds for any node that exposes ROS2 parameters and implements the paramter callback. 

The configuartion `.yaml` files are extensively commented to understand what each paramteres refers to, the accepted values, and how they are mapped to the sonar behavior. 

### Bonus) Other Useful Commands and Tools

- **Recording ros2 bags**<br/>

    To **record a specific subset of topics**, in anoyther terminal you can use the command:
    ```
    ros2 bag record /topic1 /topic2
    ```

    **TAKE CARE**: If the full oculus stack is running, including the point cloud and high resolution cartesian images, bag files can be EXTREMELY heavy if all messages are stored. 

    For example, the most important data published during sonar usage are stored in `/ping` message. The `/ping` topic 
    in fact stores all the acoustic echo recorded by the oculus receiver, that can be then processed with the `sonar_viewer` node.  
    If `the oculus_sonar_node` is running with the `sonar` namespace, you can store the ping messages as: 

    ```
    ros2 bag record /sonar/ping
    ```

    Then, replaying this bag with the `sonar_viewer_node` running you will have access to all the visualization functionality of the driver library. The same for the other sonar nodes that process the raw ping data or the cartesian/polar images. 

    (Useful tutorial on working with ros2 bags [here](https://roboticsbackend.com/ros2-bag-save-and-replay-topic-data/))


- **Converting ros2 bags to Images**<br/>

    Once recorder a ROS2 bag, you may want to save the polar/cartesian sonar frames in a local directory as .png images. <br/>
    The `sonar_saver_node` has been implemented exactly for this. <br/>
    
    The code is not yet optimized to be device agnostic in handling the absolute/relative paths, so you need to make some minor modification on the node header to properly configur ethe save path. 
    Additionally, some subscribers and publisher topic names are "hardcoded" in the nodes used in the steps above, take care of properly remapping the topic names and namespaces if needed. 

    0) Open [sonar_saver.yaml](OCULUS_ROS2/oculus_ros2/include/oculus_ros2/cfg/sonar_saver.yaml) and update the
       `output_dir` and `bag_name` parameters with your absolute desired save directory. The polar and cartesian images will be saved in `output_dir/bag_name/`<br/>
    1) Since you apply changes on the source code, remember to rebuild the ws and source. 
    2) Start the sonar saver node 
       ```
       ros2 run oculus_ros2 sonar_saver_node --ros-args --params-file </path/to/sonar_saver_yaml>.yaml
       ```     
    3) IF the bag store only the ping messages (with raw sonar echo readings). Start the viewer node flor image formation (polar and cartesian)
       ```
       ros2 run oculus_ros2 sonar_viewer_node
       ```    
    4) play the bag file of interest 
       ```
       ros2 bag play <bag_name>
       ``` 
    It is possible to peroform the steps above by launching a single launch file, taking care of properly updating the bag path, topic names and possible namespaces of the nodes: 

    ```
    ros2 launch oculus_ros2 oculus_ros2_bag_saver.launch.py bag_path:=<path/to/bag>
    ```
    
- **Post process and inspect ros2 bag messages**<br/>

    The launch file ``oculus_ros2_bag`` provide a good template launcher for any sonar data processing based on recorded bag. <br/>
    It automatically play the specified bag in loop mode, start the viewer node for image processing, rviz2 for visualization, and some other utility nodes. <br/>

    Please notice that some topics are remapped because of timestamp WARNINGS reason. 
    Probably there is a better way to handle the different timestamp from the running nodes and the messages broadcasted by the bag file. 

    ```
    ros2 launch oculus_ros2 oculus_ros2_bag.launch.py bag_path:=<absolute_path_to_bag>
    ```

## Sonar Data Brief Description 

Preliminary notes on how the Oculus M3000d Sonar generate the acoustic images readings and how they are stored in the ROS2 driver.<br/>
To read more about the Data Structure Definitions, read the official [documentation file](docs/NT-148-D00527-06_Oculus_Data_Structure_Definitions.pdf)

### Oculus Network Communication 

As indicated in the sdk [documentation file](docs/NT-148-D00527-06_Oculus_Data_Structure_Definitions.pdf), the sensor communicate with the 
processor (computer or embedded system) with both UDP and TCP. <br/>

**UDP Socket (Port: 52102)**<br/>
Used for the status diagram message at 1 Hz. Stores sensor IP and health information. 

The UDP port number is used by the low level oculus_driver to retrieve the IP and establish TCP connection.
This is done once the driver class is initialized in the `oculus_sonar_node` set up.

**TCP Socket (Port: 52100)**<br/>
Channel for control signals and sensor data. <br/>
The sonar pinging works for a limited amount of time, therefore a new request have to be transmitted periodically to keep the imaging alive at the pingRate requested.

### Oculus SDK Messages (Relevant Ones)

To see the overall source code of the proprietary oculus Messages, explore the msg definition in [Oculus.h](./oculus_driver_lib/include/oculus_driver/Oculus.h) package, and the wrappers in [OculusMessage.h](./oculus_driver_lib/include/oculus_driver/OculusMessage.h).


### Oculus ROS2 Messages (Relevant Ones)

To see the overall source code of the custom oculus ROS2 Messages, explore the `msg` definition in the [oculus_interfaces](OCULUS_ROS2/oculus_interfaces) package.  

- [`Ping`](./OCULUS_ROS2/oculus_interfaces/msg/Ping.msg): Complete information about sonar ping return, including raw acoustic return matrix
- [`OculusPing2`](./OCULUS_ROS2/oculus_interfaces/msg/OculusPing2.msg): 
- [`OculusStatus`](./OCULUS_ROS2/oculus_interfaces/msg/OculusStatus.msg):  

### Acoustic Image Formation Process 

Generic highoverall level description of the logic behind acoustic images formation process. 

### Oculus Sonar Logic

Acoustic image formation process for the **oculus blueprint M3000d** series sonar. The same logic is used by previous version of the M series forward looking sonar, with some changes in the software implementation on the sdk. 
<br/>(Probably other FLS sensors uses similar workflows)



## Oculus M3000d in Depth

More detailed hardware and software information to use or modify/improve the M3000d driver. 


### About the M3000d Hardware (Available Informations)

This section provide some useful details on the Oculus M3000d hardware. <br/>
Some of those details can be relevant for any sensor fusion algorithm or to better understand the available data and processing steps. 

### Integrated Proprioceptive Sensors (Attitute Heading + Pressure)

Additionally to the transducers used to generate the acoustic image (transmit sound and record echo), the sonar integrate additional proprioceptive sensors.  

In particular, the M3000d integrate an Attitude and Heading Reference System (AHRS) and Pressure Sensor (snesors' model and specifications not available) <br/>
The images below show the location and axis configuration of the AHRS (shared by the BluePrintSubsea Technical support team).

<image width=480 height=360 src=docs/images/M3000d_AHRS_location.png>
<image width=480 height=360 src=docs/images/M3000d_AHRS_axis.png>

**IMPORTANT**: The raw data of the IMU used by the AHRS are not exposed by the sdk.
Roll, Pitch and Heading are stored in the oculus Ping message. <br/>
From some preliminary tests those data are not reliable, and calibration is not possible. 

With respect to the pressure information, additional tests are required to estimate its quantitative performances. 
The **pressure** data is stored also in the oculus Ping message, in **[Pa]**. <br/>
Calibrating the sensor for the static bias and filtering the noise, it should provide accurate pressure measurements for sonar depth. 


## Software Structure Notes

Software structure of the low level C++ Sonar Driver and the overlay ROS2 Interface. 

Here the high level details such as design choices and software relations are explained. Refer to the comment on the source code for further details on the implementation choices.  

### Low Level Driver (oculus_driver)

C++ library to interface with the Sonar Hardware.

Expose classes to connect, configure and read the Sonar data. 

### ROS2 Driver

ROS2 Jazzy (C++) packages to interface with the Oculus M3000d in a ROS2 infrastructure. 

Handling sonar initial configuration and dynamic reconfigure, UDP connection, sensor read, raw sensor read processing, and polar image conversion to cartesian.

Other features will be integrated according to research task requrements. 

### oculus_interface

[This package](OCULUS_ROS2/oculus_interfaces/) contains the custom ROS2 sonar messages interfaces. <br/>
It allows to exchange in the ROS network equivalent informatioon to the ones used by the propritary software.

These ROS2 messages wrap the official oculus [SDK](./oculus_driver_lib/include/oculus_driver/OculusMessage.h) messages data structures, 
converted by [conversions.hpp](./OCULUS_ROS2/oculus_ros2/include/oculus_ros2/conversions.hpp)


### oculus_ros2

[This package](OCULUS_ROS2/oculus_ros2/) handles the interface with the sonar hardware and the data processing. 


#### [``conversions.hpp``](/OCULUS_ROS2/oculus_ros2/include/oculus_ros2/conversions.hpp)

This utils code implements conversions from oculus messages to ROS2 messages.

When the ``oculus_sonar_node`` receives the sonar messages from the low level driver instance, it rely on these methods to ROSify them and publish on the ROS network.  


#### [``oculus_sonar_node``](OCULUS_ROS2/oculus_ros2/include/oculus_ros2/oculus_sonar_node.hpp)

This Node handles the interface with the sonar hardware thorugh an instance of the low level sonar driver class. 

It has the follwoing **main tasks**: 
- Connect to the sonar via UDP 
- Sett the initial configuration of the sensor (frequency mode, range, gain, etc..)
- ROSify sonar measurements and broadcast to the nework 
- Handle reconfiguartion requests

The sensor configuration is handled by this node ROS parameters, initialized by a config file: [oculus_sonar.yaml](/oculus_ros2/cfg/oculus_sonar.yaml)

The node implements the parameter reconfiguration callback, sending **dynamic reconfiguration** request to the sonar. <br/>
NOTE: This part is still under development and adjustments.


#### [``sonar_viewer_node``](OCULUS_ROS2/oculus_ros2/include/oculus_ros2/sonar_viewer_node.hpp)

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


#### [``sonar_point_cloud_node``](OCULUS_ROS2/oculus_ros2/include/oculus_ros2/sonar_point_cloud_node.hpp)
This Node publishes a point cloud computed from the cartesian sonar image. <br/>
The points are selcted as the ones with pixel intensity (echo intensity) higher than a specified user threshold. 
Due to the acoustic image formation model, this naive solution form a point cloud in sonar zero elevation plane (where the sonar frustum is projected). 


#### [``sonar_state_estimation_node``](OCULUS_ROS2/oculus_ros2/include/oculus_ros2/sonar_state_estimation_node.hpp)

This Node exploits the proprioceptive sonar information (depth and orientation) coming from a depth sensor and an AHRS.

After the sensor readings, the pose (depth position + orientation) is published as a ROS TransForm frame (TF). <br/>
This allow a visualization of the sonar frame in rviz (a 3D textured model and a sensor URDF description is defined in the package)


#### [``sonar_saver_node``](OCULUS_ROS2/oculus_ros2/include/oculus_ros2/sonar_saver_node.hpp)

This Node should be runned to save **all** the cartesian and polar acoustic images inside of a specified output directory. 

It uses the ROS cv_bridge utils to convert ROS images in the OpenCV format, ready to be saved in the desired image compression format.  



## Problems and Possible Improvements

List of software/hardware known problems, bugs and ideas for further improvements. 

### Problems and Bugs to Solve

- **Changing the Gain in High Frequency Mode** <br/>
    You may be surprised when reconfguring the gain in high frequency mode, the number that you will set won't be the one reflected in the ros2 parameter. <br/>
    This is what is stated in the SonarDruver source code of the low level `oculus_driver_lib`: <br/>
    ```
    // When masterMode = 2, the sonar force gainPercent between 40& and
    // 100%, BUT still needs resquested gainPercent to be between 0%
    // and 100%. (If you request a gainPercent=0 in masterMode=2, the
    // fireMessage in the ping results will be 40%). The gainPercent is
    // rescaled here to ensure consistent parameter handling on client
    // side).
    if(newConfig.masterMode == 2) {
        newConfig.gainPercent = (newConfig.gainPercent - 40.0) * 100.0 / 60.0;
    }
    ```
    As you can see, a proper value rescaling is performed when sending the request to the sonar. <br/>
    I haven't tested the behavior without this yet. 

- **Reconfiguring Sensor Publish Rate** <br/>
    From the last tests it looks like the sensor publish rate is limited to maximum 7fps even when reconfiguring it to higher publish frequency.
    This is a relevant problem that must be solved to operate the sonar efficiently in the field

- **Running Sonar + IP Cam** <br/>
    When both the sonar and ip camera (sony) run simultaneously, it looks like they are overloading the ethernet communication and multiple frames 
    of the sonar are lost periodically. This is a relevant issue to address in order to deploy both camera and sonar in the field. 

### To Test 

- **Sonar + USB Cam**<br/>
    The same problem of the IP Camera occurs?


### To Do and Ideas 

- **Implement Robust Sonar Image Filtering** <br/>
   It is mandatory before implementing any perception algorithms to find optimal ways to filter out as much noise as possible from the sonar data. Artifacts and Speckle noise will influence and degradate the performance of any compuer vision and deep learning technique. 