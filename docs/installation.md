# Installation

Tested on Ubuntu 24.04 + ROS2 Jazzy.

## 0) Setting up the workspace

### 1. Install and set up ROS2 Jazzy

:::{dropdown} Install and set up ROS2 Jazzy

**1.1) Install ROS2 Jazzy and source it.**
Refer to the [ROS2 documentation](https://docs.ros.org/en/jazzy/Installation/Ubuntu-Install-Debs.html)
for the step-by-step installation tutorial.

Any time you open a new terminal, source your underlay ROS environment:

```bash
# Replace ".bash" with your shell if you're not using bash
# Possible values are: setup.bash, setup.sh, setup.zsh
source /opt/ros/jazzy/setup.bash
```

To avoid doing it every time, add that command to your `~/.bashrc`:

```bash
gedit ~/.bashrc   # or your preferred text editor
```

and add the line `source /opt/ros/jazzy/setup.bash` at the end.

**1.2) Install `colcon`** (if not already installed), the build tool for ROS2:

```bash
sudo apt install python3-colcon-common-extensions
```

**1.3) Create and build a ROS2 workspace** to hold this project's packages, in your preferred directory:

```bash
mkdir sonar_ws
cd sonar_ws
mkdir src
colcon build
```

**1.4) Source your ROS2 workspace and the colcon argcomplete environment.**
As done for the ROS underlay, you need to source the overlay `setup.bash` once it is built.
Again, you can add it to your `~/.bashrc`:

```bash
source ~/sonar_ws/install/setup.bash
source /usr/share/colcon_argcomplete/hook/colcon-argcomplete.bash
```

The second line enables completion of colcon commands with the Tab key.

**1.5) Install additional packages.**
To launch this project's packages you may need ROS2 packages that are not installed with
`ros-jazzy-desktop`:

- `xacro`
- `image_transport`

```bash
sudo apt install ros-${ROS_DISTRO}-xacro ros-${ROS_DISTRO}-image-transport
```
:::

### 2. Create the ROS2 workspace

```bash
mkdir sonar_ws && cd sonar_ws
```

### 3. Create the `src` directory and build

```bash
mkdir src
colcon build
```

### 4. Clone or copy this repo into `src`

```bash
cd src
git clone https://github.com/AlePuglisi/oculus-ros2-cpp-driver.git
```

### 5. Move into the driver directory for the next steps

```bash
cd oculus-ros2-cpp-driver
```

## 1) Install the `oculus_driver` library

### 1. Install dependencies

```bash
sudo apt-get install -y libboost-system-dev libboost-thread-dev
```

### 2. Set up the build

This library follows a standard CMake compilation procedure.
From the cloned repository, move to the library directory and create a build directory:

```bash
cd oculus_driver_lib
mkdir build && cd build
```

### 3. Build the `oculus_driver` library

Generate the build system with CMake and compile:

```bash
cmake ..
make -j4
```

:::{dropdown} With more explicit build commands

```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/usr/local \
      .. && \
make -j$(nproc) && \
make install
```
:::

### 4. Install the driver

```bash
sudo make -j4 install
```

This installs the driver at the system level (in `/usr/local/lib`), not locally in the workspace.
If you want the driver installed only inside the workspace, modify the
[`oculus_driver_lib` CMakeLists.txt](https://github.com/AlePuglisi/oculus-ros2-cpp-driver/blob/main/oculus_driver_lib/CMakeLists.txt),
or use the explicit build commands above with `-DCMAKE_INSTALL_PREFIX` pointing to a different install path.

:::{note}
If the `oculus_driver` library is installed elsewhere, the `oculus_ros2` `CMakeLists.txt` may also need
changes to find the library.
:::

## 2) Build the `oculus_ros2` package

If all the steps above compiled without errors, you have a ROS2 workspace ready to be built and the
`oculus_driver` library properly installed.

Building the ROS2 packages (`oculus_interfaces` + `oculus_ros2`) is the usual procedure. From the
workspace root directory (e.g. `~/sonar_ws`), run:

```bash
colcon build
```

## Integrating new features and understanding the driver

If you want to add new features or understand the software structure, see
[Software Structure Notes](software_structure.md).
