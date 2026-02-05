#include <oculus_ros2/sonar_point_cloud_node.hpp>

SonarPointCloud::SonarPointCloud()
    : Node("sonar_point_cloud")
{
    // Declare parameters
    this->declare_parameter<double>("pixels_per_meter", 50.0);
    this->declare_parameter<int>("intensity_threshold", 125);
    this->declare_parameter<bool>("use_16bit", true);
    this->declare_parameter<double>("intensity_threshold_16bit", 32000.0);
    this->declare_parameter<bool>("flipped_fan_v", false);

    // Get parameters
    this->pixels_per_meter_ = this->get_parameter("pixels_per_meter").as_double();
    this->intensity_threshold_ = this->get_parameter("intensity_threshold").as_int();
    this->use_16bit_ = this->get_parameter("use_16bit").as_bool();
    this->intensity_threshold_16bit_ = this->get_parameter("intensity_threshold_16bit").as_double();
    this->flipped_fan_v_ = this->get_parameter("flipped_fan_v").as_bool();

    RCLCPP_INFO(this->get_logger(), "Point Cloud Parameters:");
    RCLCPP_INFO(this->get_logger(), "pixels_per_meter: %.1f", this->pixels_per_meter_);
    RCLCPP_INFO(this->get_logger(), "intensity_threshold: %d", this->intensity_threshold_);
    RCLCPP_INFO(this->get_logger(), "use_16bit: %s", this->use_16bit_ ? "true" : "false");
    RCLCPP_INFO(this->get_logger(), "flipped_fan_v: %s", this->flipped_fan_v_ ? "true" : "false");

    this->x_offset_ = 0.0; 
    this->y_offset_ = 0.0; 

    this->world_sonar_rotation_ << 1, 0, 0, 0, 1, 0, 0, 0, 1; 
    this->world_sonar_translation_ << 0, 0, 0; 

    ping_subscriber_ = this->create_subscription<oculus_interfaces::msg::Ping>(
        "ping", 1, std::bind(&SonarPointCloud::ping_callback_, this, std::placeholders::_1));
    
    cartesian_image_subscriber_ = this->create_subscription<sensor_msgs::msg::Image>(
        "fan_image", 1, std::bind(&SonarPointCloud::cartesian_image_callback_, this, std::placeholders::_1));
    
    point_cloud_publisher_ = this->create_publisher<sensor_msgs::msg::PointCloud2>(
        "sonar_point_cloud", 1);

    callback_handle_ = this->add_on_set_parameters_callback(
        std::bind(&SonarPointCloud::parametersCallback, this, std::placeholders::_1));

    // Initialize tf2 related variables
    tf_buffer_ = std::make_unique<tf2_ros::Buffer>(this->get_clock());
    tf_listener_ = std::make_shared<tf2_ros::TransformListener>(*tf_buffer_);

}

SonarPointCloud::~SonarPointCloud()
{
    RCLCPP_INFO(this->get_logger(), "Shutting down Sonar Point Cloud Node");
}

rcl_interfaces::msg::SetParametersResult SonarPointCloud::parametersCallback(
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
        else if (param.get_name() == "intensity_threshold") {
            this->intensity_threshold_ = param.as_int();
            RCLCPP_INFO(this->get_logger(), "Updated intensity_threshold: %d", this->intensity_threshold_);
        }
        else if (param.get_name() == "intensity_threshold_16bit") {
            this->intensity_threshold_16bit_ = param.as_double();
            RCLCPP_INFO(this->get_logger(), "Updated intensity_threshold_16bit: %.1f", this->intensity_threshold_16bit_);
        }
        if (param.get_name() == "use_16bit")
        {
            RCLCPP_INFO_STREAM(this->get_logger(), "Updating use_16bit from " << this->use_16bit_ << " to " << param.as_bool() << std::endl);
            this->use_16bit_ = param.as_bool();
        }
        if (param.get_name() == "flipped_fan_v")
        {
            RCLCPP_INFO_STREAM(this->get_logger(), "Updating flipped_fan_v from " << this->flipped_fan_v_ << " to " << param.as_bool() << std::endl);
            this->flipped_fan_v_ = param.as_bool();
        }
    }
    
    return result;
}

void SonarPointCloud::ping_callback_(const oculus_interfaces::msg::Ping & sonar_ping_msg)
{
    // Store ping info for potential use in point cloud generation
    last_ping_msg_ = sonar_ping_msg;
    has_ping_data_ = true;
}

void SonarPointCloud::cartesian_image_callback_(const sensor_msgs::msg::Image & image_cartesian_msg)
{
    
    const int width  = image_cartesian_msg.width;
    const int height = image_cartesian_msg.height;
  
    this->y_offset_ = - (width / 2)  / this->pixels_per_meter_; 
    this->x_offset_ = height / this->pixels_per_meter_; 

    float flip_v = 1.0; 
    if (this->flipped_fan_v_){
        flip_v = -1.0; 
        this->x_offset_ += - height / this->pixels_per_meter_;
    }

    // Determine if image is 8-bit or 16-bit
    bool is_16bit = (image_cartesian_msg.encoding == sensor_msgs::image_encodings::MONO16);
    
    RCLCPP_INFO_ONCE(this->get_logger(), "Cartesian image: %dx%d, encoding: %s", 
                     width, height, image_cartesian_msg.encoding.c_str());

    auto point_cloud_msg = sensor_msgs::msg::PointCloud2();
    point_cloud_msg.header = image_cartesian_msg.header;
    point_cloud_msg.header.frame_id = "sonar"; 
    point_cloud_msg.is_dense = false;

    // Setup point cloud with XYZ and intensity fields
    sensor_msgs::PointCloud2Modifier modifier(point_cloud_msg);
    modifier.setPointCloud2FieldsByString(1, "xyz");
    modifier.resize(width * height); // Pre-allocate maximum possible

    sensor_msgs::PointCloud2Iterator<float> iter_x(point_cloud_msg, "x");
    sensor_msgs::PointCloud2Iterator<float> iter_y(point_cloud_msg, "y");
    sensor_msgs::PointCloud2Iterator<float> iter_z(point_cloud_msg, "z");
    //sensor_msgs::PointCloud2Iterator<float> iter_intensity(point_cloud_msg, "intensity");

    size_t point_count = 0;

    updateWorldSonarTransform_(); 

    if (is_16bit) {
        // ====================================================================
        // 16-bit Image Processing (MONO16)
        // ====================================================================
        const uint16_t* data = reinterpret_cast<const uint16_t*>(image_cartesian_msg.data.data());

        for (int row = 0; row < height; row++) {
            for (int col = 0; col < width; col++) {
                
                uint16_t intensity_16 = data[row * width + col];
                
                // Threshold filtering
                if (intensity_16 < this->intensity_threshold_16bit_) {
                    continue;
                }
                
                // ============================================================
                // PIXEL TO METRIC COORDINATE CONVERSION
                // ============================================================
                // The Cartesian image was created with:
                //   x_pixel = (x_metric - x_min) * pixels_per_meter
                //   y_pixel = height - 1 - (y_metric - y_min) * pixels_per_meter
                //
                // Therefore, reverse transformation:
                //   x_metric = x_pixel / pixels_per_meter + x_min
                //   y_metric = (height - 1 - y_pixel) / pixels_per_meter + y_min
                // ============================================================

                float x_m = (-row * flip_v ) / this->pixels_per_meter_ + this->x_offset_;
                float y_m = col / this->pixels_per_meter_ + this->y_offset_;
                float z_m = 0.0f; // XY plane for now

                Eigen::Vector3d point(x_m, y_m, z_m); 
                //point = this->world_sonar_rotation_ * point + this->world_sonar_translation_; 

                // Add point to cloud
                // *iter_x = x_m;
                // *iter_y = y_m;
                // *iter_z = z_m;
                *iter_x = point(0);
                *iter_y = point(1);
                *iter_z = point(2);

                //*iter_intensity = static_cast<float>(intensity_16);
                
                ++iter_x;
                ++iter_y;
                ++iter_z;
                //++iter_intensity;
                ++point_count;
            }
        }
    }
    else {
        // ====================================================================
        // 8-bit Image Processing (MONO8)
        // ====================================================================
        const uint8_t* data = reinterpret_cast<const uint8_t*>(image_cartesian_msg.data.data());
        
        for (int row = 0; row < height; row++) {
            for (int col = 0; col < width; col++) {
                
                uint8_t intensity_8 = data[row * width + col];
                
                // Threshold filtering
                if (intensity_8 < this->intensity_threshold_) {
                    continue;
                }
                
                // Convert pixel to metric coordinates (same formula as above)
                float x_m = -row / this->pixels_per_meter_ + this->x_offset_;
                float y_m = col / this->pixels_per_meter_ + this->y_offset_;
                float z_m = 0.0f;
                

                // Add point to cloud
                *iter_x = x_m;
                *iter_y = y_m;
                *iter_z = z_m;
                //*iter_intensity = static_cast<float>(intensity_8);
                
                ++iter_x;
                ++iter_y;
                ++iter_z;
                //++iter_intensity;
                ++point_count;
            }
        }
    }

    // Resize to actual number of points
    modifier.resize(point_count);
    point_cloud_msg.width = point_count;
    point_cloud_msg.height = 1; // Unorganized point cloud

    RCLCPP_DEBUG(this->get_logger(), "Published point cloud with %zu points", point_count);
    
    point_cloud_publisher_->publish(point_cloud_msg);
}

void SonarPointCloud::publishPointCloud_(const oculus_interfaces::msg::Ping & sonar_ping_msg, 
    const sensor_msgs::msg::Image & image_cartesian_msg)
{
    // Optional: Use ping data to enhance point cloud
    // (e.g., add range information, timestamp, etc.)
    
    // For now, just call the cartesian image callback
    cartesian_image_callback_(image_cartesian_msg);
}

void SonarPointCloud::updateWorldSonarTransform_()
{
    try {
        // for now rely on tf, then from rpy + depth data 
        this->world_sonar_tf_ = tf_buffer_->lookupTransform(
            "world",               // Reference frame
            "sonar",               // Target frame
            tf2::TimePointZero     // Latest available transform
            );

        Eigen::Isometry3d transform_matrix = tf2::transformToEigen(world_sonar_tf_); 
        this->world_sonar_rotation_ = transform_matrix.rotation(); 
        this->world_sonar_translation_ = transform_matrix.translation(); 
    } 
    catch (tf2::TransformException &ex){
        RCLCPP_INFO(this->get_logger(), "%s", ex.what());
    }
}


int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<SonarPointCloud>());
    rclcpp::shutdown();
    return 0;
}