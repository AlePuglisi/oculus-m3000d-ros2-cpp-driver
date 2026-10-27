# Software Structure Notes

Software structure of the low-level C++ sonar driver and the overlay ROS2 interface.

This page explains the high-level details, such as design choices and software relations. Refer to the
comments in the source code for further details on implementation choices.

## Low-level driver (`oculus_driver`)

C++ library to interface with the sonar hardware. It exposes classes to connect to, configure and read
data from the sonar.

## ROS2 driver

ROS2 Jazzy (C++) packages to interface with the Oculus M3000d in a ROS2 infrastructure. They handle:

- sonar initial configuration and dynamic reconfiguration,
- UDP connection,
- sensor reading and raw reading processing,
- polar-to-cartesian image conversion.

Other features will be integrated according to research task requirements.

## `oculus_interfaces`

[This package](./../OCULUS_ROS2/oculus_interfaces/)
contains the custom ROS2 sonar message interfaces. It allows exchanging, over the ROS network,
information equivalent to the one used by the proprietary software.

These ROS2 messages wrap the official Oculus
[SDK - OculusMessage.h](./../oculus_driver_lib/include/oculus_driver/OculusMessage.h)
message data structures, converted by
[`conversions.hpp`](./../OCULUS_ROS2/oculus_ros2/oculus_ros2/include/oculus_ros2/conversions.hpp).

## `oculus_ros2`

[This package](./../OCULUS_ROS2/oculus_ros2/)
handles the interface with the sonar hardware and the data processing.

### `conversions.hpp`

These utilities implement the conversions from Oculus messages to ROS2 messages. When the
`oculus_sonar_node` receives the sonar messages from the low-level driver instance, it relies on these
methods to ROSify them and publish them on the ROS network.

### `oculus_sonar_node`

This node handles the interface with the sonar hardware through an instance of the low-level sonar
driver class.
([header](./../OCULUS_ROS2/oculus_ros2/include/oculus_ros2/oculus_sonar_node.hpp))

Its **main tasks** are:

- connect to the sonar via UDP,
- set the initial configuration of the sensor (frequency mode, range, gain, etc.),
- ROSify sonar measurements and broadcast them to the network,
- handle reconfiguration requests.

The sensor configuration is handled by this node's ROS parameters, initialized from a config file:
[`oculus_sonar.yaml`](./../OCULUS_ROS2/oculus_ros2/cfg/oculus_sonar.yaml).

The node implements the parameter reconfiguration callback, sending **dynamic reconfiguration** requests
to the sonar.

:::{note}
This part is still under development and adjustment.
:::

### `sonar_viewer_node`

([sonar_viewer_node.hpp](./../OCULUS_ROS2/oculus_ros2/include/oculus_ros2/sonar_viewer_node.hpp))

This node processes the raw sonar data (as a bearings-bins matrix) to create a polar and a cartesian
acoustic image.

It allows physics-based pre-processing of the raw data. In the proprietary visualization software
(Oculus ViewPoint), as in other sonars, some software logic is used to correct the image stream for a
pleasant view: what you see is usually a processed version of the raw acoustic echo. Moreover, the gain
is also adapted by the transducers during image formation, and each bin can be adjusted with a different
gain value.

The process of changing the gain applied to each bin (range) based on echo intensity and other factors
is called **Adaptive Gain Control** (AGC). These range gains are included in the sonar readings.

For these reasons, this node lets you decide whether to retrieve the original raw echo by "removing" the
broadcast AGC gains. Afterwards, custom pre-processing can be applied to the retrieved raw data.
The node also provides a **transmission loss correction**, which applies a time-propagation and water
absorption loss gain to the acoustic data.

These features are handled by ROS parameters (also allowing online reconfiguration):

- set `remove_agc` to `true` to override the applied transducer range gains and retrieve the raw data;
- set `apply_tvg` to `true` to apply the custom physical correction gain.

:::{warning}
Be careful how you combine the two pre-processing features. Applying the TVG without removing the AGC
(`remove_agc = false`, `apply_tvg = true`) saturates the sensor readings, because the correction gain is
applied twice (both AGC and transmission loss).
:::

### `sonar_point_cloud_node`

([sonar_point_cloud_node.hpp](./../OCULUS_ROS2/oculus_ros2/include/oculus_ros2/sonar_point_cloud_node.hpp))

This node publishes a point cloud computed from the cartesian sonar image. The points are selected as
the ones with a pixel intensity (echo intensity) higher than a user-specified threshold. Due to the
acoustic image formation model, this naive solution forms a point cloud in the sonar zero-elevation
plane (where the sonar frustum is projected).

### `sonar_state_estimation_node`

([sonar_state_estimation_node.hpp](./../OCULUS_ROS2/oculus_ros2/include/oculus_ros2/sonar_state_estimation_node.hpp))

This node exploits the proprioceptive sonar information (depth and orientation) coming from the depth
sensor and the AHRS.

After reading the sensors, the pose (depth position + orientation) is published as a ROS transform
frame (TF). This allows visualizing the sonar frame in rviz2 (a 3D textured model and a sensor URDF
description are defined in the package).

:::{note}
This node is a prototype and is currently not reliable (see
[M3000d in Depth](m3000d_in_depth.md)).
:::

### `sonar_saver_node`

([sonar_saver_node.hpp](./../OCULUS_ROS2/oculus_ros2/include/oculus_ros2/sonar_saver_node.hpp))

This node should be run to save **all** the cartesian and polar acoustic images inside a specified
output directory. It uses the ROS `cv_bridge` utilities to convert ROS images to the OpenCV format,
ready to be saved in the desired image compression format.
