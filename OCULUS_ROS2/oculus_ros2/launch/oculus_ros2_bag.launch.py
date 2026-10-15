import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.substitutions import Command, LaunchConfiguration
from launch.actions import ExecuteProcess, DeclareLaunchArgument

def generate_launch_description():

    bag_path_arg = DeclareLaunchArgument(
        'bag_path',
        default_value='/home/user/rosbag2',
        description='Absolute ROS2 bag path'
    )
    bag_path = LaunchConfiguration('bag_path')

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

    # Bag playback process
    bag_play_process = ExecuteProcess(
        cmd=['ros2', 'bag', 'play', '--loop', bag_path],
        output='screen'
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
            ('fan_image', 'fan_image/now'),
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
   
    robot_state_publisher_node = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        parameters=[{'robot_description': robot_description_command}]
    ) 

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

    jointstate_publisher_gui = Node(
        package='joint_state_publisher_gui',
        executable='joint_state_publisher_gui',
        output="screen" 
    )


    return LaunchDescription([
        bag_path_arg,
        bag_play_process,
        oculus_sonar_node,
        sonar_viewer_node,
        sonar_point_cloud_node,
        world_frame_static_tf_node,
        sonar_frame_static_tf_node,
        robot_state_publisher_node, 
        rviz_node
        #sonar_state_estimation_node,
        #sonar_optical_flow_node,
        #imaging_sonar_optical_flow_pose_node,
        #sonar_saver_node,
        #jointstate_publisher_gui

    ])