// sonar_point_cloud_node.hpp

#ifndef OCULUS_ROS2__SONAR_POINT_CLOUD_NODE_HPP_
#define OCULUS_ROS2__SONAR_POINT_CLOUD_NODE_HPP_

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <sensor_msgs/point_cloud2_iterator.hpp>
#include <sensor_msgs/image_encodings.hpp>
#include <oculus_interfaces/msg/ping.hpp>

#include <Eigen/Dense>
#include <Eigen/StdVector>
#include <Eigen/Eigenvalues> 
#include <tf2/LinearMath/Quaternion.h>
#include <tf2/LinearMath/Matrix3x3.h>

#include "tf2_ros/buffer.h"
#include "tf2_ros/transform_listener.h"
#include "tf2_eigen/tf2_eigen.hpp"
#include "geometry_msgs/msg/transform_stamped.hpp"

class SonarPointCloud : public rclcpp::Node
{
public:
    SonarPointCloud();
    ~SonarPointCloud();

private:
    // Callbacks
    void ping_callback_(const oculus_interfaces::msg::Ping & msg);
    void cartesian_image_callback_(const sensor_msgs::msg::Image & msg);
    void publishPointCloud_(const oculus_interfaces::msg::Ping & sonar_ping_msg, 
                           const sensor_msgs::msg::Image & image_cartesian_msg);

    void updateWorldSonarTransform_();

    rcl_interfaces::msg::SetParametersResult parametersCallback(
        const std::vector<rclcpp::Parameter> &parameters);

    // Subscribers
    rclcpp::Subscription<oculus_interfaces::msg::Ping>::SharedPtr ping_subscriber_;
    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr cartesian_image_subscriber_;
    
    // Publishers
    rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr point_cloud_publisher_;
    
    // Parameters
    rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr callback_handle_;
    
    // Configuration
    double pixels_per_meter_;
    double x_offset_;
    double y_offset_;
    int intensity_threshold_;
    bool use_16bit_;
    bool flipped_fan_v_;
    double intensity_threshold_16bit_;
    
    // State
    oculus_interfaces::msg::Ping last_ping_msg_;
    bool has_ping_data_ = false;

    // tf2 tools
    std::unique_ptr<tf2_ros::Buffer> tf_buffer_;
    std::shared_ptr<tf2_ros::TransformListener> tf_listener_{nullptr};

    geometry_msgs::msg::TransformStamped world_sonar_tf_;
    Eigen::Matrix3d world_sonar_rotation_; 
    Eigen::Vector3d world_sonar_translation_; 

};

#endif  // OCULUS_ROS2__SONAR_POINT_CLOUD_NODE_HPP_