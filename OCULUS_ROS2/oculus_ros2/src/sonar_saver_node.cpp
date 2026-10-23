#include <oculus_ros2/sonar_saver_node.hpp>

SonarSaver::SonarSaver()
    : Node("sonar_saver")
{
    // Declare parameters
    this->declare_parameter<double>("pixels_per_meter", 100.0);
    this->declare_parameter<std::string>("dataset_name", "year_month_day_hour_minute_second");
    this->declare_parameter<std::string>("output_path", "/path/to/images/dir/");

    // Get parameters
    this->pixels_per_meter_ = this->get_parameter("pixels_per_meter").as_double();
    this->output_path_ = this->get_parameter("output_path").as_string();
    this->dataset_name_ = this->get_parameter("dataset_name").as_string();

    this->create_output_dir_(output_path_, dataset_name_);

    //RCLCPP_INFO(this->get_logger(), "Sonar Saver Parameters:");

    // Set Up Subscribers
    ping_subscriber_ = this->create_subscription<oculus_interfaces::msg::Ping>(
        "ping", 1, std::bind(&SonarSaver::ping_callback_, this, std::placeholders::_1));
    
    cartesian_image_subscriber_ = this->create_subscription<sensor_msgs::msg::Image>(
        "fan_image", 1, std::bind(&SonarSaver::cartesian_image_callback_, this, std::placeholders::_1));

    polar_image_subscriber_ = this->create_subscription<sensor_msgs::msg::Image>(
        "rtheta_image", 1, std::bind(&SonarSaver::polar_image_callback_, this, std::placeholders::_1));

    // Set Up Parameter Callback
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

void SonarSaver::create_output_dir_(std::string output_path, std::string dataset_name)
{
    this->output_dir_ = output_path + dataset_name + "/sonar/";

    if (std::filesystem::exists(output_dir_))
        std::filesystem::remove_all(output_dir_);

    std::filesystem::create_directories(output_dir_ + "polar/");
    std::filesystem::create_directories(output_dir_ + "cartesian/");

    RCLCPP_INFO(this->get_logger(), "Created directory to store images");

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
        ss << output_dir_ << "cartesian/" << image_cartesian_msg.header.stamp.sec << "_" << image_cartesian_msg.header.stamp.nanosec << ".png";
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
        ss << output_dir_ << "polar/" << image_polar_msg.header.stamp.sec << "_" << image_polar_msg.header.stamp.nanosec << ".png";
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