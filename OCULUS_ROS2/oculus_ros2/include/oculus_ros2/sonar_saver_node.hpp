// sonar_saver_node.hpp

#ifndef OCULUS_ROS2__SONAR_POINT_CLOUD_NODE_HPP_
#define OCULUS_ROS2__SONAR_POINT_CLOUD_NODE_HPP_

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/image_encodings.hpp>
#include <oculus_interfaces/msg/ping.hpp>

#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <cv_bridge/cv_bridge.hpp>

#include "rcl_interfaces/msg/set_parameters_result.hpp"

#include <sstream>
#include <iomanip>

#define OUTPUT_DIR "/home/ale/sonar_ws/src/oculus-ros2-cpp-driver/OCULUS_ROS2/Data/images/sonar/2026_02_06_14_15/"

class SonarSaver : public rclcpp::Node
{
public:
    SonarSaver();
    ~SonarSaver();

private:
    // Callbacks
    void ping_callback_(const oculus_interfaces::msg::Ping & msg);
    void cartesian_image_callback_(const sensor_msgs::msg::Image & msg);
    void polar_image_callback_(const sensor_msgs::msg::Image & msg);

    rcl_interfaces::msg::SetParametersResult parametersCallback(
        const std::vector<rclcpp::Parameter> &parameters);

    // Subscribers
    rclcpp::Subscription<oculus_interfaces::msg::Ping>::SharedPtr ping_subscriber_;
    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr cartesian_image_subscriber_;
    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr polar_image_subscriber_;
    
    // Parameters
    rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr callback_handle_;

    // Configuration
    double pixels_per_meter_;

    // State
    oculus_interfaces::msg::Ping last_ping_msg_;
    bool has_ping_data_ = false;
    

};

#endif  // OCULUS_ROS2__SAVER_NODE_HPP_