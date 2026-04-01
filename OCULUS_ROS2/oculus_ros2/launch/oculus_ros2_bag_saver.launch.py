import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.substitutions import LaunchConfiguration
from launch.actions import ExecuteProcess, DeclareLaunchArgument

def generate_launch_description():

    viewer_config = os.path.join(
        get_package_share_directory("oculus_ros2"), "cfg", "sonar_viewer.yaml"
    )

    bag_path_arg = DeclareLaunchArgument(
        'bag_path',
        default_value='/home/user/rosbag2',
        description='Absolute ROS2 bag path'
    )
    bag_path = LaunchConfiguration('bag_path')

    sonar_viewer_node = Node(
        package="oculus_ros2",
        executable="sonar_viewer_node",
        name="sonar_viewer",
        parameters=[viewer_config],
        namespace="sonar",
        output="screen",
    )

    sonar_saver_node = Node(
        package="oculus_ros2",
        executable="sonar_saver_node",
        name="sonar_saver",
        namespace="sonar",
        output="screen",
    )

    # Add the bag playback process
    bag_play_process = ExecuteProcess(
        cmd=['ros2', 'bag', 'play', bag_path],
        output='screen'
    )

    return LaunchDescription([

        bag_path_arg,

        bag_play_process,
        sonar_viewer_node,
        sonar_saver_node
    ])
