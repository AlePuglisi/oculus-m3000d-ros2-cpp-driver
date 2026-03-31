cmake_minimum_required(VERSION 3.16)
project(oculus_ros2 VERSION 1.0)
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -ggdb3)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(ament_cmake_python REQUIRED) 
find_package(rclcpp REQUIRED)
find_package(rclpy REQUIRED)
find_package(rcl_interfaces REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(oculus_driver REQUIRED)
find_package(oculus_interfaces REQUIRED)
find_package(cv_bridge REQUIRED)
find_package( OpenCV REQUIRED )
find_package(tf2_ros REQUIRED)
find_package(tf2 REQUIRED)
find_package(tf2_eigen REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(Eigen3 REQUIRED)

## oculus_sonar_node ##
add_executable(oculus_sonar_node
    src/oculus_sonar_node.cpp
)

target_include_directories(oculus_sonar_node PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(oculus_sonar_node PRIVATE
    oculus_driver
    ${OpenCV_LIBS} 
)
ament_target_dependencies(oculus_sonar_node PUBLIC
    rclcpp
    oculus_interfaces
    rcl_interfaces
    sensor_msgs
    OpenCV
    cv_bridge
)

set_target_properties(oculus_sonar_node PROPERTIES
    BUILD_RPATH "/usr/local/lib"
    INSTALL_RPATH "/usr/local/lib"
)

## sonar_viewer_node ##
add_executable(sonar_viewer_node 
    src/sonar_viewer_node.cpp
)

target_include_directories(sonar_viewer_node PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

ament_target_dependencies(sonar_viewer_node PUBLIC
    rclcpp
    oculus_interfaces
    rcl_interfaces
    sensor_msgs
    OpenCV
    cv_bridge
)

## sonar_point_cloud_node ##
add_executable(sonar_point_cloud_node 
    src/sonar_point_cloud_node.cpp
)

target_include_directories(sonar_point_cloud_node PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${EIGEN3_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)

ament_target_dependencies(sonar_point_cloud_node PUBLIC
    rclcpp
    oculus_interfaces
    rcl_interfaces
    sensor_msgs
    tf2_ros
    tf2
    tf2_eigen
)

## sonar_state_estimation_node ##
add_executable(sonar_state_estimation_node 
    src/sonar_state_estimation_node.cpp
)

target_include_directories(sonar_state_estimation_node PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${EIGEN3_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)

ament_target_dependencies(sonar_state_estimation_node PUBLIC
    rclcpp
    oculus_interfaces
    rcl_interfaces
    sensor_msgs
    tf2_ros
    tf2
    tf2_eigen
)

## sonar_saver_node ##
add_executable(sonar_saver_node 
    src/sonar_saver_node.cpp
)

target_include_directories(sonar_saver_node PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${OpenCV_INCLUDE_DIRS}>
    $<INSTALL_INTERFACE:include>
)

ament_target_dependencies(sonar_saver_node PUBLIC
    rclcpp
    oculus_interfaces
    rcl_interfaces
    sensor_msgs
    OpenCV
    cv_bridge
)


## install 
ament_python_install_package(${PROJECT_NAME})

install(PROGRAMS 
            scripts/display_oculus_file.py 
            scripts/oculus_to_rosbag.py 
            scripts/oculus_subscriber_to_image.py
            scripts/oculus_optical_flow.py
            scripts/imaging_sonar_optical_flow_pose.py
        DESTINATION lib/${PROJECT_NAME})


install(DIRECTORY launch cfg mesh urdf DESTINATION share/${PROJECT_NAME})

install(TARGETS oculus_sonar_node DESTINATION lib/${PROJECT_NAME})
install(TARGETS sonar_viewer_node DESTINATION lib/${PROJECT_NAME})
install(TARGETS sonar_point_cloud_node DESTINATION lib/${PROJECT_NAME})
install(TARGETS sonar_state_estimation_node DESTINATION lib/${PROJECT_NAME})
install(TARGETS sonar_saver_node DESTINATION lib/${PROJECT_NAME})

ament_package()
