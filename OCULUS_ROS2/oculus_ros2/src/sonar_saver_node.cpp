#include <oculus_ros2/sonar_saver_node.hpp>

SonarSaver::SonarSaver()
    : Node("sonar_saver")
{
    // Declare parameters
    this->declare_parameter<double>("pixels_per_meter", 250.0);

    // Get parameters
    this->pixels_per_meter_ = this->get_parameter("pixels_per_meter").as_double();

    //RCLCPP_INFO(this->get_logger(), "Sonar Saver Parameters:");

    ping_subscriber_ = this->create_subscription<oculus_interfaces::msg::Ping>(
        "ping", 1, std::bind(&SonarSaver::ping_callback_, this, std::placeholders::_1));
    
    cartesian_image_subscriber_ = this->create_subscription<sensor_msgs::msg::Image>(
        "fan_image", 1, std::bind(&SonarSaver::cartesian_image_callback_, this, std::placeholders::_1));

    polar_image_subscriber_ = this->create_subscription<sensor_msgs::msg::Image>(
        "rtheta_image", 1, std::bind(&SonarSaver::polar_image_callback_, this, std::placeholders::_1));

    callback_handle_ = this->add_on_set_parameters_callback(
        std::bind(&SonarSaver::parametersCallback, this, std::placeholders::_1));

}

SonarSaver::~SonarSaver()
{
    RCLCPP_INFO(this->get_logger(), "Shutting down Sonar Saver Node");
}

rcl_interfaces::msg::SetParametersResult SonarSaver::parametersCallback(
    const std::vector<rclcpp::Parameter> &parameters)
{
    rcl_interfaces::msg::SetParametersResult result;
    result.successful = true;
    result.reason = "success";
    
    for (const auto &param: parameters)
    {
        if (param.get_name() == "pixels_per_meter") {
            this->pixels_per_meter_ = param.as_double();
            RCLCPP_INFO(this->get_logger(), "Updated pixels_per_meter: %.1f", this->pixels_per_meter_);
        }
    }
    
    return result;
}

void SonarSaver::ping_callback_(const oculus_interfaces::msg::Ping & sonar_ping_msg)
{
    // Store ping info for potential use in point cloud generation
    this->last_ping_msg_ = sonar_ping_msg;
    this->has_ping_data_ = true;

}

void SonarSaver::cartesian_image_callback_(const sensor_msgs::msg::Image & image_cartesian_msg)
{
    try{
        cv::Mat cartesian_frame = cv_bridge::toCvCopy(image_cartesian_msg, "mono16")->image;

        std::string image_path;
        std::stringstream ss; 
        ss << OUTPUT_DIR << "cartesian/" << image_cartesian_msg.header.stamp.sec << "_" << image_cartesian_msg.header.stamp.nanosec << ".png";
        image_path = ss.str();

        cv::imwrite(image_path, cartesian_frame);
    }
    catch(cv_bridge::Exception& e){
        RCLCPP_ERROR(this->get_logger(), "cv_bridge exception: %s", e.what());
    }
}

void SonarSaver::polar_image_callback_(const sensor_msgs::msg::Image & image_polar_msg)
{
    try{
        cv::Mat cartesian_frame = cv_bridge::toCvCopy(image_polar_msg, "mono16")->image;

        std::string image_path;
        std::stringstream ss; 
        ss << OUTPUT_DIR << "polar/" << image_polar_msg.header.stamp.sec << "_" << image_polar_msg.header.stamp.nanosec << ".png";
        image_path = ss.str();

        cv::imwrite(image_path, cartesian_frame);
    }
    catch(cv_bridge::Exception& e){
        RCLCPP_ERROR(this->get_logger(), "cv_bridge exception: %s", e.what());
    }
}

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<SonarSaver>());
    rclcpp::shutdown();
    return 0;
}