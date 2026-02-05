#ifndef OCULUS_ROS2__SONAR_VIEWER_NODE_HPP_
#define OCULUS_ROS2__SONAR_VIEWER_NODE_HPP_

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"
#include "sensor_msgs/msg/compressed_image.hpp"
#include <oculus_interfaces/msg/ping.hpp>

#include <cv_bridge/cv_bridge.hpp>

#include <opencv2/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "rcl_interfaces/msg/set_parameters_result.hpp"

class SonarViewer : public rclcpp::Node {
public:
    SonarViewer();
    ~SonarViewer();

private:

    bool remove_agc_;           // Flag for Adaptive Gain Control (AGC) removal
    bool apply_tvg_;            // Flag for Time Variable Gain (TVG) application 
    double gain_applied_;       // Gain applied on sonar echo (ovveride sonar gain)
    double pixels_per_meter_;   // pixels/meter used in Fan (cartesian) image resolution
    bool enable_fan_grid_;      // Flag for Fan grid application (TODO: apply on an opencv image rather than ROS)
    bool flip_vertically_;      // Flag for Flipping sonar image vertically

    rclcpp::Subscription<oculus_interfaces::msg::Ping>::SharedPtr ping_subscriber_;               // ROS Subsciber to Oculus Ping
    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr rtheta_publisher_;                      // ROS Publihser of Polar Image
    rclcpp::Publisher<sensor_msgs::msg::CompressedImage>::SharedPtr rtheta_compressed_publisher_; // ROS Publihser to Compressed Polar Image
    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr xy_publisher_;                          // ROS Publihser to Cartesian Image


    OnSetParametersCallbackHandle::SharedPtr callback_handle_;  // ROS callback handler for paramter update
    
    /**
     * @brief Callback on Paramter Update
     *
     * @param parameters  
     * 
     */
    rcl_interfaces::msg::SetParametersResult parametersCallback_(
        const std::vector<rclcpp::Parameter> &parameters); 
    /**
     * @brief Callback on Oculus Ping messages 
     *
     * @param ping_msg 
     * 
     */
    void pingCallback_(const oculus_interfaces::msg::Ping & ping_msg); 
    /**
     * @brief Process polar to cartesian conversion and publish it 
     *
     * @param ping_msg
     * @param image_polar_msg
     * 
     */
    void publishFan_(const oculus_interfaces::msg::Ping & ping_msg, 
                    const sensor_msgs::msg::Image & image_polar_msg);
    /**
     * @brief (WIP)Utility to add Grid lines (points) on cartesian image
     *
     * @param range_max
     * @param bearing_min
     * @param bearing_max
     * @param x_min
     * @param y_min
     * 
     */
    void addGridLines(sensor_msgs::msg::Image& image, 
                    double range_max,
                    double bearing_min, 
                    double bearing_max,
                    double x_min,
                    double y_min);
    /**
     * @brief Utility to compute Transmission Loss of Sonar
     *
     * @param range: range of interest [m]
     * @param alpha: absorption coefficient (0.3 at 1MHz in seawater) [dB/m]
     * 
     */                    
    float TL_M3000d_(float range, float alpha); 
};

/**
 * @brief Utility to compress ROS Image message
 *
 * @param image_msg
 * 
 */   
sensor_msgs::msg::CompressedImage compressImageMsg(const sensor_msgs::msg::Image & image_msg); 

#endif  // OCULUS_ROS2__SONAR_VIEWER_NODE_HPP_