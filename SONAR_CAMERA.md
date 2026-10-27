# Notes on ROS2 Packages to work with Sonar + Camera + UW Sensors 

Useful "Remember" and Notes on testing and developing ROS2 packages to work with sensors

# Sonar

The usage of the sonar rely on the `oculus_ros2` package. Currently there are not critical problems in the driver. 

# Camera 

Notes and details on working with camera and ROS2. 

The current biggest issue is the communication delay in reading camera frames. 

## Compressing-Uncompressing Camera Images 

Install Image transport plugin packages to handle compression of images 
```
sudo apt install ros-${ROS_DISTRO}-image-transport
```

To Compress camera images published in `image_raw/`>



To Uncompress previously compressed camera images published in `image_raw/compressed`>
```
ros2 run image_transport republish compressed in/compressed:=image_raw/compressed raw out:=image_raw/uncompressed
```

Or maybe? 

```
ros2 run image_transport republish \
    compressed \
    raw \
    --ros-args \
    -r in/compressed:=/image_topic/compressed  \
    -r out:=/image_topic/image_raw
``` 

## SONY IP Camera 
### About Communication Lag (Streaming Delay)

The standard python script based on cv videocapture with queue is extremely delayed. Using the new node based not on opencv the synchronization is better but the image quality is not guaranteed. 

### ROS2 package - cam_ros2 

**python nodes**

python node (based on opencv videocapture on rtps)

```
ros2 run cam_ros2 ip_cam_node_py
```

(The `ip_cam_queue_node_py` accumulate an incredible amount of delay, it is unusable because it doesn't discard images)


**C++ nodes**

The `ip_cam_node` whih doesn't rely on opencv but on GStreamer is so far the best in terms of delay (some milliseconds). But there is a clear image degradation when the motion is fast or the obsatcles are close. 


```
ros2 run cam_ros2 ip_cam_node
```

On the other hand, the `ip_cam_cv_node`, which rely on OpenCV has better quality than the GStreamer one, but keep some streaming delay (0.5 - 1 second).

```
ros2 run cam_ros2 ip_cam_cv_node
```

### ROS2 package - ros2_ipcamera

I tried to debug the original node (which was not working). 
But still there is at least 1-2 seconds of delay. 


## (Logitech) USB Camera 

### With standard usb cam node (using v4l2 transport)

[Official Documentation](https://docs.ros.org/en/jazzy/p/v4l2_camera/)

List Video devices (even without ROS2).
<br/>
Install v4l-utils:
```
sudo apt install v4l-utils
```
List devices connected
```
v4l2-ctl --list-devices
```

Install v4l2 ros2 node
```
sudo apt-get update
sudo apt-get install ros-${ROS_DISTRO}-v4l2-camera
```

Run the node with custom parameters<br/>
With command line parameters
```
ros2 run v4l2_camera v4l2_camera_node --ros-args -p video_device:=/dev/video2
```

With Configuration file parameters
```
ros2 run v4l2_camera v4l2_camera_node --ros-args --params-file custom_camera_parameters.yaml
```



Where the available ROS2 Parameters are: 
- `video_device` : `/dev/video0`
- `image_size` : `[640, 480]`
- `pixel_format` : `mjpeg` or `YUYV`
- `output_encoding` : `rgb8`
- `framerate` : `30.0`

### v4l2_camera from source 
In case relevant node modification is required and source code has to be used and modified: 

```
git clone --branch ${ROS_DISTRO} https://gitlab.com/boldhearts/ros2_v4l2_camera.git src/v4l2_camera
rosdep install --from-paths src/v4l2_camera --ignore-src -r -y
colcon build
```
Read the [blog post](https://medium.com/swlh/raspberry-pi-ros-2-camera-eef8f8b94304) for further details. It focus on using the node with Raspberry Pi.  

# Sensors Synchronization (USB Cam + Sonar)

An approach to synchronize the sensors readings is from the ROS2 timestamp. It can be fine if you don't have hard time constraints.

A useful tool to check messages rate in ros2 is the rqt_bag gui 

```
ros2 run rqt_bag rqt_bag <bag_path>
```
This allow to visualize in a timeline when ros2 messages arrives. However is better to check the difference in the sensor clock and sonar publish timestamp to be sure that real synchronization is happening. 

# Sensors Synchronization<br/>(USB Cam + Sonar + IMU/Compass/Pressure)

How to synchronize all the sensors in the robot?