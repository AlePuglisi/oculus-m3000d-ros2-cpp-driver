# Oculus Sonar Docker Setup

Quick setup guide for running the Oculus M3000d sonar driver in Docker.

## Prerequisites

### All Platforms
- Docker installed and running
- Oculus sonar connected via Ethernet
- Sonar and host computer on same subnet (e.g., both 192.168.1.x)

### Windows Only
- **VcXsrv X Server** for GUI support
- Download: https://sourceforge.net/projects/vcxsrv/

### Linux Only
- X11 (usually pre-installed)

---

## Quick Start

### On Linux (Ubuntu/Raspberry Pi/Jetson)

```bash
# 1. Navigate to docker directory
cd ~/sonar_ws/src/oculus-m3000d-ros2-cpp-driver/docker

# 2. Make script executable (first time only)
chmod +x run_linux.sh

# 3. Run the container
./run_linux.sh
```

The script will:
- Check Docker is running
- Configure X11 for GUI support
- Build the image (first time only, ~5-10 minutes)
- Start the container
- Drop you into a bash shell inside the container

### On Windows

```batch
REM 1. Start VcXsrv first!
REM    - Launch XLaunch
REM    - Display number: 0
REM    - Check "Disable access control"

REM 2. Navigate to docker directory
cd C:\path\to\oculus-m3000d-ros2-cpp-driver\docker

REM 3. Run the container
run_windows.bat
```

---

## Inside the Container

Once inside, you're in a Linux environment with ROS2 and all dependencies installed.

### Launch the Sonar Driver

```bash
ros2 launch oculus_ros2 <launch_file.launch.py>
```

### View Sonar Data

Open a new terminal and enter the container:
```bash
# On host (Linux)
docker exec -it oculus_sonar_driver bash

# Or on host (Windows)
docker exec -it oculus_sonar_driver bash
```

Then inside:
```bash
# List topics
ros2 topic list

# View ping data
ros2 topic echo /oculus_sonar/ping

# Launch RViz
rviz2

# View/change parameters
ros2 param list
ros2 param get /oculus_sonar ping_rate
ros2 param set /oculus_sonar ping_rate 2
```

### Exit the Container

```bash
exit
```

---

## Network Setup

Your computer and sonar must be on the same subnet.

### Find Sonar IP

The sonar broadcasts its IP. Inside the container:
```bash
# Install tcpdump if needed
apt-get update && apt-get install -y tcpdump

# Listen for sonar broadcasts (port 52102)
tcpdump -i any port 52102
```

### Configure Your Network

**Example:** If sonar is at `192.168.1.10`:

#### Linux
```bash
# Check current IP
ip addr show

# Add IP to ethernet interface (if needed)
sudo ip addr add 192.168.1.100/24 dev eth0
```

#### Windows
1. Settings → Network → Ethernet → Properties
2. IPv4 Settings
3. Set IP: `192.168.1.100`
4. Subnet: `255.255.255.0`

---

## Common Commands

### Start Container (Background)
```bash
cd docker/
docker-compose up -d
```

### Access Running Container
```bash
docker exec -it oculus_sonar_driver bash
```

### Stop Container
```bash
docker-compose down
```

### View Logs
```bash
docker logs -f oculus_sonar_driver
```

### Rebuild Image (After Code Changes)
```bash
cd docker/
docker-compose build
docker-compose up -d
```

---

## Troubleshooting

### No GUI on Linux

```bash
# Grant X11 access
xhost +local:docker

# Check DISPLAY variable
echo $DISPLAY  # Should show :0 or :1

# Test GUI
docker exec -it oculus_sonar_driver xcalc
```

### No GUI on Windows

1. Verify VcXsrv is running (check system tray)
2. Check Windows Firewall allows VcXsrv
3. Inside container, test:
   ```bash
   echo $DISPLAY  # Should show host.docker.internal:0.0
   xcalc  # Should open calculator
   ```

### Cannot Find Sonar

```bash
# Inside container, check network
ip addr show

# Ping sonar (replace with your sonar IP)
ping 192.168.1.10

# Check if sonar is broadcasting
tcpdump -i any port 52102
```

### Build Fails

Make sure you're in the `docker/` directory:
```bash
pwd  # Should show: .../oculus-m3000d-ros2-cpp-driver/docker
ls ..  # Should show: oculus_driver_lib, oculus_ros2, docker, etc.
```

---

## What's Happening?

When you run the container:

1. **Build stage** (first time only):
   - Builds C++ driver from `../oculus_driver_lib/`
   - Installs to `/usr/local/lib` (inside container)
   - Builds ROS2 packages from `../oculus_ros2/`
   - Installs to `/ros2_ws` (inside container)

2. **Runtime**:
   - Container runs Ubuntu 24.04 with ROS2 Jazzy
   - Shares your network (can access sonar)
   - Shares X11 (can show GUI)
   - Everything isolated from your host system

3. **Your files stay on host**:
   - Windows: `C:\path\to\oculus-m3000d-ros2-cpp-driver\`
   - Linux: `~/sonar_ws/src/oculus-m3000d-ros2-cpp-driver/`
   - Docker copies them during build

---

## Development Workflow

### Edit Code

1. Edit files on your host machine (Windows/Linux)
2. Rebuild Docker image:
   ```bash
   cd docker/
   docker-compose down
   docker-compose build
   docker-compose up -d
   ```

### Quick Rebuild (Inside Container)

For faster iteration during development:

```bash
# Enter container
docker exec -it oculus_sonar_driver bash

# Rebuild ROS2 workspace
cd /ros2_ws
colcon build --symlink-install
source install/setup.bash

# Test changes
ros2 launch oculus_ros2 helios.launch.py
```

## File Structure

```
docker/
├── Dockerfile              # Multi-stage build configuration
├── docker-compose.yaml     # Container orchestration
├── run_linux.sh            # Linux launcher script
├── run_windows.bat         # Windows launcher script
├── COLCON_IGNORE           # Prevents ROS2 from building this folder
└── README.md               # This file
```

Build context is parent directory (`..`) to access:
- `../oculus_driver_lib/` - C++ driver library
- `../oculus_ros2/` - ROS2 packages


