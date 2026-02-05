// sonar_point_cloud_node.hpp

#ifndef OCULUS_ROS2__SONAR_STATE_ESTIMATION_NODE_HPP_
#define OCULUS_ROS2__SONAR_STATE_ESTIMATION_NODE_HPP_

#include <rclcpp/rclcpp.hpp>
#include <oculus_interfaces/msg/ping.hpp>
#include <oculus_interfaces/msg/oculus_ping2.hpp>
#include <oculus_interfaces/msg/oculus_pose_estimation.hpp>

#include <Eigen/Dense>
#include <Eigen/StdVector>
#include <Eigen/Eigenvalues> 
#include <tf2/LinearMath/Quaternion.h>
#include <tf2/LinearMath/Matrix3x3.h>

#include "tf2_ros/buffer.h"
#include "tf2_ros/transform_broadcaster.h"
#include "tf2_eigen/tf2_eigen.hpp"
#include "geometry_msgs/msg/transform_stamped.hpp"


#include <cmath>


namespace phy 
{
    const double GRAVITY       = 9.80665;   // [m/s^2]
    const double SEA_DENSITY   = 1023.6;    // [kg/m^3]
    const double FRESH_DENSITY = 997.0474;   // [kg/m^3]

    /**
     * Calculate seawater density using UNESCO EOS-80 equation
     * 
     * @param S Salinity (PSU or g/kg), typical range 0-42
     * @param T Temperature (°C), typical range -2 to 40
     * @param P Pressure (dbar), where 1 dbar ≈ 1 meter depth
     * @return Density (kg/m³)
     */
    double seawater_density(double S, double T, double P); 

}


class SonarStateEstimation : public rclcpp::Node
{
public:
    SonarStateEstimation();
    ~SonarStateEstimation();

private:
    // Callbacks
    void oculusPingCallback_(const oculus_interfaces::msg::OculusPing2 & oculus_ping_msg);

    void updateWorldSonarTransform_(const oculus_interfaces::msg::OculusPoseEstimation & oculus_pose_msg);

    rcl_interfaces::msg::SetParametersResult parametersCallback(
        const std::vector<rclcpp::Parameter> &parameters);

    // Subscribers
    rclcpp::Subscription<oculus_interfaces::msg::OculusPing2>::SharedPtr oculus_ping_subscriber_;
    
    // Publishers
    rclcpp::Publisher<oculus_interfaces::msg::OculusPoseEstimation>::SharedPtr oculus_pose_publisher_;
    
    // Parameters
    rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr callback_handle_;
    
    // Configuration
    double roll_;
    double pitch_;
    double heading_;
    double depth_;

    double salinity_; 
    double water_density_; 
    bool use_accurate_density_; 

    // tf tools 
    geometry_msgs::msg::TransformStamped world_sonar_tf_;
    Eigen::Matrix3d world_sonar_rotation_; 
    Eigen::Vector3d world_sonar_translation_; 

    std::shared_ptr<tf2_ros::TransformBroadcaster> sonar_tf_broadcaster_{nullptr};

};

#endif  // OCULUS_ROS2__SONAR_STATE_ESTIMATION_NODE_HPP_