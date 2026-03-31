# BSD 3-Clause License
#
# Copyright (c) 2022, ENSTA-Bretagne
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived from
#    this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.substitutions import Command
from launch.actions import ExecuteProcess

def generate_launch_description():

    ld = LaunchDescription()

    sonar_config = os.path.join(
        get_package_share_directory("oculus_ros2"), "cfg", "oculus_sonar.yaml"
    )

    viewer_config = os.path.join(
        get_package_share_directory("oculus_ros2"), "cfg", "sonar_viewer.yaml"
    )

    point_cloud_config = os.path.join(
        get_package_share_directory("oculus_ros2"), "cfg", "sonar_point_cloud.yaml"
    )

    pose_state_config = os.path.join(
        get_package_share_directory("oculus_ros2"), "cfg", "sonar_state_estimation.yaml"
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
            ('rtheta_image', 'rtheta_image/now'),
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
        remappings=[
            ('rtheta_image', 'rtheta_image/now'),
            ('fan_image', 'fan_image/now'),
        ]
    )

    sonar_saver_node = Node(
        package="oculus_ros2",
        executable="sonar_saver_node",
        name="sonar_saver",
        namespace="sonar",
        output="screen",
        remappings=[
            ('fan_image', 'fan_image/now'),
            ('rtheta_image', 'rtheta_image/now'),
        ]
    )

    sonar_point_cloud_node = Node(
        package="oculus_ros2",
        executable="sonar_point_cloud_node",
        name="sonar_point_cloud",
        parameters=[point_cloud_config],
        namespace="sonar",
        output="screen",
        remappings=[
            ('sonar_point_cloud', 'sonar_point_cloud/now'),
        ]
    )

    sonar_state_estimation_node = Node(
        package="oculus_ros2",
        executable="sonar_state_estimation_node",
        name="sonar_state_estimation",
        parameters=[pose_state_config],
        namespace="sonar",
        output="screen",
    )
    
    sonar_frame_static_tf_node = Node(
            package="tf2_ros",
            executable="static_transform_publisher",
            output="screen" ,
            arguments=["0", "0", "0", "0", "0", "0", "map", "sonar"]
    )

    world_frame_static_tf_node = Node(
            package="tf2_ros",
            executable="static_transform_publisher",
            output="screen" ,
            arguments=["0", "0", "0", "0", "0", "0", "map", "world"]
    )

    rviz_node =  Node(
            package='rviz2',
            namespace='',
            executable='rviz2',
            name='rviz2',
            arguments=['-d' + os.path.join(get_package_share_directory('oculus_ros2'), 'cfg', 'oculus_bag.rviz')]
        )
    
    description_path = get_package_share_directory('oculus_ros2')
    #xacro_path = os.path.join(description_path, "urdf", "sonar_6dof.urdf.xacro")
    xacro_path = os.path.join(description_path, "urdf", "sonar.urdf.xacro")
    robot_description_command = Command(['xacro ', xacro_path])

    sonar_optical_flow_node = Node(
        package='oculus_ros2',
        executable='oculus_optical_flow.py',
        namespace="sonar",
        output="screen",
        remappings=[
            ('fan_image', 'fan_image/now'),
        ]
    ) 

    imaging_sonar_optical_flow_pose_node = Node(
        package='oculus_ros2',
        executable='imaging_sonar_optical_flow_pose.py',
        namespace="sonar",
        output="screen",
        remappings=[
            ('fan_image', 'fan_image/now'),
        ]
    ) 

    robot_state_publisher_node = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        parameters=[{'robot_description': robot_description_command}]
    ) 

    # jointstate_publisher_gui = Node(
    #     package='joint_state_publisher_gui',
    #     executable='joint_state_publisher_gui',
    #     output="screen" 
    # )

    # Add the bag playback process
    bag_play_process = ExecuteProcess(
        cmd=['ros2', 'bag', 'play', '--loop', 
             '/home/ale/sonar_data/rosbag2_2026_02_06-15_14_30'],
        output='screen'
    )

    ld.add_action(bag_play_process) 
    
    ld.add_action(oculus_sonar_node)
    ld.add_action(sonar_viewer_node)    
    #ld.add_action(sonar_optical_flow_node)    
    # ld.add_action(imaging_sonar_optical_flow_pose_node)    
    #ld.add_action(sonar_saver_node)
    # ld.add_action(sonar_point_cloud_node)
    # ld.add_action(sonar_state_estimation_node)
    ld.add_action(world_frame_static_tf_node)
    ld.add_action(sonar_frame_static_tf_node)
    ld.add_action(robot_state_publisher_node)
    #ld.add_action(jointstate_publisher_gui)

    ld.add_action(rviz_node)

    return ld
