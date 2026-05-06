import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.substitutions import Command


def generate_launch_description():

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

    sonar_point_cloud_node = Node(
        package="oculus_ros2",
        executable="sonar_point_cloud_node",
        name="sonar_point_cloud",
        parameters=[point_cloud_config],
        namespace="sonar",
        output="screen",
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
            arguments=['-d' + os.path.join(get_package_share_directory('oculus_ros2'), 'cfg', 'oculus.rviz')]
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
      
    jointstate_publisher_gui = Node(
        package='joint_state_publisher_gui',
        executable='joint_state_publisher_gui',
        output="screen" 
    )


    return LaunchDescription([
        oculus_sonar_node,
        sonar_viewer_node,
        sonar_point_cloud_node,
        #sonar_state_estimation_node,
        sonar_frame_static_tf_node,
        world_frame_static_tf_node,
        robot_state_publisher_node,
        rviz_node
    ])