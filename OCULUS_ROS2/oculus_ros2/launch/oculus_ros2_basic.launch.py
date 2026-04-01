import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():


    sonar_config = os.path.join(
        get_package_share_directory("oculus_ros2"), "cfg", "oculus_sonar.yaml"
    )
    viewer_config = os.path.join(
        get_package_share_directory("oculus_ros2"), "cfg", "sonar_viewer.yaml"
    )

    oculus_sonar_node = Node(
        package="oculus_ros2",
        executable="oculus_sonar_node",
        name="oculus_sonar",
        parameters=[sonar_config],
        namespace="sonar",
        remappings=[
            ("status", "status"),
            ("ping", "ping"),
            ("temperature", "temperature"),
            ("pressure", "pressure"),
        ],
        output="screen",
    )

    sonar_viewer_node = Node(
        package="oculus_ros2",
        executable="sonar_viewer_node",
        name="sonar_viewer",
        parameters=[viewer_config],
        namespace="sonar",
        output="screen",
    )
      

    return LaunchDescription([
        oculus_sonar_node,
        sonar_viewer_node,
    ])