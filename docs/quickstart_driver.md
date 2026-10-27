# Quick Start: Using the Driver

To start the Oculus M3000d driver, different launch files are available for basic functionality or
more "advanced" features.

:::{important}
You cannot start the ROS2 driver while Oculus ViewPoint is running.
If you launch any of the ROS2 driver scripts while ViewPoint is running, it will not work.
:::

:::{important}
Make sure no firewall is active on your device. A firewall can cause connection problems both in
ViewPoint and with the ROS2 driver. You can disable it from a terminal with:

```bash
sudo ufw disable
```
:::

## 0) Define the initial configuration

If you run one of the launch files, you can set the initial node parameter configuration in a `.yaml`
file. Navigate to the
[`oculus_ros2/oculus_ros2/cfg`](https://github.com/AlePuglisi/oculus-ros2-cpp-driver/tree/main/oculus_ros2/oculus_ros2/cfg)
directory, where you can find the configuration file used by each node.

Setting up the driver and processing nodes in advance is not crucial, since dynamic parameter
reconfiguration is handled by the nodes, as described below.

## 1) Start the driver and optional sonar processing nodes

### Sonar driver

To run only the driver node (handling the ROS2 interface with the low-level sonar driver library):

```bash
ros2 run oculus_ros2 oculus_sonar_node
```

:::{dropdown} Exposed topics

If the driver runs without any topic or namespace remapping (as in the command above), it exposes:

- `/status`: high-level status information about the sensor state, device ID, IP and other information, remapped to other topics for easy access.
- `/ping`: encodes the overall ping return in a readable way, including processed sound received, timestamp, etc.
- `/oculus_ping`: encodes the overall ping return.
- `/temperature`: internal electronics temperature. Used to enter protection mode when the temperature is too high.
- `/pressure`: pressure to which the sensor is subjected. Used to retrieve depth for sound speed computation.

Additionally, the reconfigurable ROS2 node parameters are exposed to change the sonar configuration
(frequency mode, gain, etc.). See [Reconfiguring the sonar](#2-reconfiguring-the-sonar-and-the-processing-nodes) below.
:::

### Sonar driver + image formation

```bash
ros2 launch oculus_ros2 oculus_ros2_basic.launch.py
```

This runs the `oculus_sonar_node` and the `sonar_viewer_node`.

### Sonar driver + image formation + point cloud + rviz2

```bash
ros2 launch oculus_ros2 oculus_ros2_full.launch.py
```

## 2) Reconfiguring the sonar and the processing nodes

Most of the configuration of the sonar driver and the processing nodes (such as the viewer and the
point cloud publisher) is handled by ROS2 node parameters. This allows reconfiguration during sensor
usage, without restarting the process (using the parameter reconfiguration callback).

You can reconfigure the parameters with the ROS2 CLI tools, but it is more comfortable to use the
`rqt` Dynamic Reconfigure plugin. Start it with:

```bash
rqt
```

Then open `Plugins > Configuration > Dynamic Reconfigure`.

From there, select the `oculus_sonar` driver node to see all the reconfigurable sonar parameters
(frequency mode, gain, range, etc.). In the same way, to change the polar-to-cartesian conversion
resolution or other aspects of the viewer, select the `sonar_viewer` node. This holds for any node
that exposes ROS2 parameters and implements the parameter callback.

The configuration `.yaml` files are extensively commented to explain what each parameter refers to,
the accepted values, and how they map to the sonar behavior.

## Bonus: other useful commands and tools

### Recording ROS2 bags

To record a specific subset of topics, run in another terminal:

```bash
ros2 bag record /topic1 /topic2
```

:::{caution}
If the full Oculus stack is running, including the point cloud and high-resolution cartesian images,
bag files can be **extremely** heavy if all messages are stored.
:::

The most important data published during sonar usage is stored in the `/ping` message. This
topic stores all the acoustic echo recorded by the Oculus receiver, which can then be processed with
the `sonar_viewer` node. If the `oculus_sonar_node` is running in the `sonar` namespace, you can
store the ping messages with:

```bash
ros2 bag record /sonar/ping
```

Replaying this bag with the `sonar_viewer_node` running gives you access to all the visualization
functionality of the driver library. The same applies to the other sonar nodes that process the raw
ping data or the cartesian/polar images.

A useful tutorial on working with ROS2 bags is available
[here](https://roboticsbackend.com/ros2-bag-save-and-replay-topic-data/).

### Converting ROS2 bags to images

Once you have recorded a ROS2 bag, you may want to save the polar/cartesian sonar frames in a local
directory as `.png` images. The `sonar_saver_node` has been implemented for exactly this.

:::{note}
The code is not yet optimized to be device-agnostic in handling absolute/relative paths, so you need to
make a minor modification to configure the save path. Additionally, some subscriber and publisher topic
names are hardcoded in the nodes used in the steps above, so take care to remap topic names and
namespaces if needed.
:::

1. Open `sonar_saver.yaml` (in the `cfg` directory of the `oculus_ros2` package) and update the
   `output_dir` and `bag_name` parameters with your desired absolute save directory. The polar and
   cartesian images are saved in `output_dir/bag_name/`.
2. Since you changed the configuration, rebuild the workspace and source it.
3. Start the sonar saver node:

   ```bash
   ros2 run oculus_ros2 sonar_saver_node --ros-args --params-file </path/to/sonar_saver>.yaml
   ```

4. If the bag only stores the ping messages (raw sonar echo readings), start the viewer node for image
   formation (polar and cartesian):

   ```bash
   ros2 run oculus_ros2 sonar_viewer_node
   ```

5. Play the bag file of interest:

   ```bash
   ros2 bag play <bag_name>
   ```

You can perform all the steps above with a single launch file, taking care to update the bag path,
topic names and possible node namespaces:

```bash
ros2 launch oculus_ros2 oculus_ros2_bag_saver.launch.py bag_path:=<path/to/bag>
```

### Post-process and inspect ROS2 bag messages

The `oculus_ros2_bag` launch file is a good template for any sonar data processing based on a recorded
bag. It automatically plays the specified bag in loop mode and starts the viewer node for image
processing, rviz2 for visualization, and some other utility nodes.

:::{note}
Some topics are remapped because of timestamp warnings. There is probably a better way to handle the
different timestamps between the running nodes and the messages broadcast by the bag file.
:::

```bash
ros2 launch oculus_ros2 oculus_ros2_bag.launch.py bag_path:=<absolute_path_to_bag>
```
