#include <oculus_ros2/sonar_state_estimation_node.hpp>


SonarStateEstimation::SonarStateEstimation()
    : Node("sonar_state_estimation")
{
    // -- Parameters --
    // Declare parameters
    this->declare_parameter<bool>("use_accurate_density", false);
    this->declare_parameter<double>("salinity", 0.0);

    // Get parameters
    this->use_accurate_density_ = this->get_parameter("use_accurate_density").as_bool();
    this->salinity_ = this->get_parameter("salinity").as_double();


    RCLCPP_INFO(this->get_logger(), "State Estimation Parameters:");
    RCLCPP_INFO(this->get_logger(), "salinity: %f", this->salinity_);
    RCLCPP_INFO(this->get_logger(), "use_accurate_density: %s", this->use_accurate_density_ ? "true" : "false");

    // -- ROS pub/sub --
    this->oculus_ping_subscriber_ = this->create_subscription<oculus_interfaces::msg::OculusPing2>(
        "oculus_ping", 1, std::bind(&SonarStateEstimation::oculusPingCallback_, this, std::placeholders::_1));
    
    this->oculus_pose_publisher_ = this->create_publisher<oculus_interfaces::msg::OculusPoseEstimation>(
        "sonar_pose_zrpy", 1);

    this->callback_handle_ = this->add_on_set_parameters_callback(
        std::bind(&SonarStateEstimation::parametersCallback, this, std::placeholders::_1));

    // -- TF2 related tools --
    this->sonar_tf_broadcaster_ = std::make_shared<tf2_ros::TransformBroadcaster>(*this);

}

SonarStateEstimation::~SonarStateEstimation()
{
    RCLCPP_INFO(this->get_logger(), "Shutting down Sonar State Estimation Node");
}

rcl_interfaces::msg::SetParametersResult SonarStateEstimation::parametersCallback(
    const std::vector<rclcpp::Parameter> &parameters)
{
    rcl_interfaces::msg::SetParametersResult result;
    result.successful = true;
    result.reason = "success";
    
    for (const auto &param: parameters)
    {
        if (param.get_name() == "salinity") {
            this->salinity_ = param.as_double();
            RCLCPP_INFO(this->get_logger(), "Updated salinity: %.1f", this->salinity_);
        }
        if (param.get_name() == "use_accurate_density")
        {
            RCLCPP_INFO_STREAM(this->get_logger(), "Updating use_accurate_density from " << this->use_accurate_density_ << " to " << param.as_bool() << std::endl);
            this->use_accurate_density_ = param.as_bool();
        }
    }
    
    return result;
}

void SonarStateEstimation::oculusPingCallback_(const oculus_interfaces::msg::OculusPing2 & sonar_ping_msg)
{
    oculus_interfaces::msg::OculusPoseEstimation oculus_pose_msg; 

    oculus_pose_msg.header = sonar_ping_msg.header;
    oculus_pose_msg.header.stamp = this->now();

    oculus_pose_msg.roll = sonar_ping_msg.roll; 
    oculus_pose_msg.pitch = sonar_ping_msg.pitch; 
    oculus_pose_msg.yaw = sonar_ping_msg.heading; 

    double density = phy::FRESH_DENSITY; 
    double pressure = sonar_ping_msg.pressure; 

    if (this->use_accurate_density_) {
        density = phy::seawater_density(this->salinity_, sonar_ping_msg.temperature, pressure);
    }
    else if (this->salinity_ > 0.1) {
        density = phy::SEA_DENSITY; 
    }

    double depth = pressure * 100000 / (phy::GRAVITY * density); 
    oculus_pose_msg.depth = depth; 

    oculus_pose_publisher_->publish(oculus_pose_msg); 

    this->updateWorldSonarTransform_(oculus_pose_msg); 
}


void SonarStateEstimation::updateWorldSonarTransform_(const oculus_interfaces::msg::OculusPoseEstimation & oculus_pose_msg)
{
    double roll  = oculus_pose_msg.roll; 
    double pitch = oculus_pose_msg.pitch; 
    double yaw   = oculus_pose_msg.yaw; 
    double depth = oculus_pose_msg.depth; 

    this->world_sonar_tf_.header = oculus_pose_msg.header; 

    this->world_sonar_tf_.header.frame_id = "world";
    this->world_sonar_tf_.child_frame_id = "sonar";

    tf2::Quaternion q;
    q.setRPY(roll, pitch, yaw);
    this->world_sonar_tf_.transform.rotation.x = q.x();
    this->world_sonar_tf_.transform.rotation.y = q.y();
    this->world_sonar_tf_.transform.rotation.z = q.z();
    this->world_sonar_tf_.transform.rotation.w = q.w();    

    this->world_sonar_tf_.transform.translation.x = 0.0; 
    this->world_sonar_tf_.transform.translation.y = 0.0; 
    this->world_sonar_tf_.transform.translation.z = depth; 

    this->sonar_tf_broadcaster_->sendTransform(world_sonar_tf_);
}

double phy::seawater_density(double S, double T, double P) {
    // Step 1: Pure water density at atmospheric pressure (kg/m³)
    double rho_w = 999.842594 
                 + 6.793952e-2 * T
                 - 9.095290e-3 * T * T
                 + 1.001685e-4 * T * T * T
                 - 1.120083e-6 * T * T * T * T
                 + 6.536332e-9 * T * T * T * T * T;
    
    // Step 2: Density at atmospheric pressure with salinity
    double A = 8.24493e-1 
             - 4.0899e-3 * T
             + 7.6438e-5 * T * T
             - 8.2467e-7 * T * T * T
             + 5.3875e-9 * T * T * T * T;
    
    double B = -5.72466e-3
             + 1.0227e-4 * T
             - 1.6546e-6 * T * T;
    
    double C = 4.8314e-4;
    
    double rho_0 = rho_w + A * S + B * pow(S, 1.5) + C * S * S;
    
    // Step 3: Secant bulk modulus K(S,T,P)
    // Pure water terms
    double K_w = 19652.21
               + 148.4206 * T
               - 2.327105 * T * T
               + 1.360477e-2 * T * T * T
               - 5.155288e-5 * T * T * T * T;
    
    // Salinity terms for K at P=0
    double K_0 = K_w
               + (54.6746 - 0.603459 * T + 1.09987e-2 * T * T - 6.1670e-5 * T * T * T) * S
               + (7.944e-2 + 1.6483e-2 * T - 5.3009e-4 * T * T) * pow(S, 1.5);
    
    // Pressure dependence of K
    double A_w = 3.239908
               + 1.43713e-3 * T
               + 1.16092e-4 * T * T
               - 5.77905e-7 * T * T * T;
    
    double A_0 = A_w
               + (2.2838e-3 - 1.0981e-5 * T - 1.6078e-6 * T * T) * S
               + 1.91075e-4 * pow(S, 1.5);
    
    double B_w = 8.50935e-5
               - 6.12293e-6 * T
               + 5.2787e-8 * T * T;
    
    double B_0 = B_w
               + (-9.9348e-7 + 2.0816e-8 * T + 9.1697e-10 * T * T) * S;
    
    double K = K_0 + A_0 * P + B_0 * P * P;
    
    // Step 4: Final density with pressure correction
    double rho = rho_0 / (1.0 - P / K);
    
    return rho;
}

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<SonarStateEstimation>());
    rclcpp::shutdown();
    return 0;
}