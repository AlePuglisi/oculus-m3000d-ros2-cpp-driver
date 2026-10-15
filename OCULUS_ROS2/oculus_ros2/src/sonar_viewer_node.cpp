#include <oculus_ros2/sonar_viewer_node.hpp>


SonarViewer::SonarViewer()
    : Node("sonar_viewer")
{
    // Declare parameters
    this->declare_parameter<bool>("remove_agc", true);
    this->declare_parameter<bool>("apply_tvg", true);
    this->declare_parameter<double>("gain", 3000.);
    this->declare_parameter<bool>("enable_fan_grid", true);
    this->declare_parameter<bool>("flip_vertically", true);
    this->declare_parameter<double>("pixels_per_meter", 100.0);

    // Initiaize node attributes from parameters
    this->remove_agc_ = this->get_parameter("remove_agc").as_bool();
    this->apply_tvg_ = this->get_parameter("apply_tvg").as_bool();
    this->gain_applied_ = this->get_parameter("gain").as_double();
    this->enable_fan_grid_ = this->get_parameter("enable_fan_grid").as_bool();
    this->flip_vertically_ = this->get_parameter("flip_vertically").as_bool();
    this->pixels_per_meter_ = this->get_parameter("pixels_per_meter").as_double();

    // Log current configuaration of paramters (attributes) 
    RCLCPP_INFO(this->get_logger(), "removing agc: %s", this->remove_agc_ ? "true" : "false");
    RCLCPP_INFO(this->get_logger(), "applying TVG: %s", this->apply_tvg_ ? "true" : "false");
    RCLCPP_INFO(this->get_logger(), "gain: %.1f", this->gain_applied_);
    RCLCPP_INFO(this->get_logger(), "Enable Grid in Fan View: %s", this->enable_fan_grid_ ? "true" : "false");
    RCLCPP_INFO(this->get_logger(), "Flip Sonar Image Vertically: %s", this->flip_vertically_ ? "true" : "false");
    RCLCPP_INFO(this->get_logger(), "pixels/meters in Fan View: %.1f", this->pixels_per_meter_);

    if (this->apply_tvg_ && !this->remove_agc_)
      RCLCPP_INFO(this->get_logger(), "Warning, if the AGC is not removed the application of the TVG is not necessary");

    // Initialize ROS subscribers and Publihsers
    this->ping_subscriber_ = this->create_subscription<oculus_interfaces::msg::Ping>(
        "ping", 10, std::bind(&SonarViewer::pingCallback_, this, std::placeholders::_1));
    // Possible Multithread Implementation of the ping processing
    // this->ping_subscriber_ = this->create_subscription<oculus_interfaces::msg::Ping>(
    //     "ping", 10,
    //     [this](oculus_interfaces::msg::Ping::UniquePtr msg) {
    //         std::lock_guard<std::mutex> lock(queue_mutex_);
    //         if (ping_queue_.size() < 3) {  // drop if falling behind, don't accumulate
    //             ping_queue_.push(std::move(*msg));
    //         }
    //         queue_cv_.notify_one();
    //     });

    processing_thread_ = std::thread(&SonarViewer::processing_loop, this);

    this->rtheta_publisher_ = this->create_publisher<sensor_msgs::msg::Image>("rtheta_image", 1);
    this->xy_publisher_ = this->create_publisher<sensor_msgs::msg::Image>("fan_image", 1);
    this->rtheta_compressed_publisher_ = this->create_publisher<sensor_msgs::msg::CompressedImage>("compressed_rtheta_image", 1);

    this->callback_handle_ = this->add_on_set_parameters_callback(
        std::bind(&SonarViewer::parametersCallback_, this, std::placeholders::_1));
}

SonarViewer::~SonarViewer()
{
    // Possible Multithread Implementation of the ping processing
    // quit_ = true;
    // queue_cv_.notify_all();
    // if (processing_thread_.joinable())
    //     processing_thread_.join();
    RCLCPP_INFO(this->get_logger(), "Shutting down Sonar Viewer");
}

rcl_interfaces::msg::SetParametersResult SonarViewer::parametersCallback_(
        const std::vector<rclcpp::Parameter> &parameters)
{
    rcl_interfaces::msg::SetParametersResult result;
    result.successful = true;
    result.reason = "success";
    RCLCPP_INFO(this->get_logger(), "Updating parameters");
    // Update the node configuration from the parameters reconfguration
    for (const auto &param: parameters)
    {
        if (param.get_name() == "remove_agc")
        {
            RCLCPP_INFO_STREAM(this->get_logger(), "Updating remove_agc from " << this->remove_agc_ << " to " << param.as_bool() << std::endl);
            this->remove_agc_ = param.as_bool();
        }
        else if (param.get_name() == "apply_tvg")
        {
            RCLCPP_INFO_STREAM(this->get_logger(), "Updating apply_tvg_ from " << this->apply_tvg_ << " to " << param.as_bool() << std::endl);
            this->apply_tvg_  = param.as_bool();
        }
        else if (param.get_name() == "gain")
        {
            RCLCPP_INFO_STREAM(this->get_logger(), "Updating gain from " << this->gain_applied_ << " to " << param.as_double() << std::endl);
            this->gain_applied_ = param.as_double();
        }
        else if (param.get_name() == "pixels_per_meter")
        {
            RCLCPP_INFO_STREAM(this->get_logger(), "Updating pixels_per_meter_ from " << this->pixels_per_meter_ << " to " << param.as_double() << std::endl);
            this->pixels_per_meter_ = param.as_double();
        }
        else if (param.get_name() == "enable_fan_grid")
        {
            RCLCPP_INFO_STREAM(this->get_logger(), "Updating enable_fan_grid_ from " << this->enable_fan_grid_ << " to " << param.as_bool() << std::endl);
            this->enable_fan_grid_ = param.as_bool();
        }
        else if (param.get_name() == "flip_vertically")
        {
            RCLCPP_INFO_STREAM(this->get_logger(), "Updating enable_fan_grid_ from " << this->flip_vertically_ << " to " << param.as_bool() << std::endl);
            this->flip_vertically_ = param.as_bool();
        }
    }
    return result;
}

// Possible Multithread Implementation of the ping processing
// void SonarViewer::processing_loop()
// {
//     while (!quit_) {
//         oculus_interfaces::msg::Ping msg;
//         {
//             std::unique_lock<std::mutex> lock(queue_mutex_);
//             queue_cv_.wait(lock, [this] { return !ping_queue_.empty() || quit_; });
//             if (quit_) break;
//             msg = std::move(ping_queue_.front());
//             ping_queue_.pop();
//         }
//         pingCallback_(msg);  // heavy processing now off the executor thread
//     }
// }

float SonarViewer::TL_M3000d_(float range, float alpha = 0.3)
{
    // the first term stand for the geometrical divergece, and the second term stands for the absorption in the water
    // theorical transmission loss should be 40log10(range) + 2.0*alpha*range;
    // The factor of two in the absorption loss is related to the double route of sound (go and echo) 
    // here we use 10log10(range) as it was found to fit best. This difference could be explained by the amplification etc done by the sonar prior to the gain application
    return 10.0*log10(range) + 2.0*alpha*range;
}

void SonarViewer::pingCallback_(const oculus_interfaces::msg::Ping & ping_msg)
{
      auto rtheta_image = sensor_msgs::msg::Image();
      rtheta_image.header = ping_msg.header;
      rtheta_image.height = ping_msg.n_ranges;
      rtheta_image.width = ping_msg.n_beams;
      rtheta_image.encoding = sensor_msgs::image_encodings::MONO16;
      rtheta_image.step = 2*rtheta_image.width;
      auto ping_data = ping_msg.ping_data;

      const int SIZE_OF_GAIN_ = 4;

      int height = ping_msg.n_ranges;
      int width = ping_msg.n_beams;

      const int step = 2*width + SIZE_OF_GAIN_;

      // the image offset is related to the UDP packet, typically 2048 bytes (header + bearing array + padding)
      const int offset = 2048;
      //const int offset = ping_msg.image_offset; 

      float r_max= ping_msg.range;

      std::vector<uint8_t> datas;

      for (int i = 0; i < height; ++i)
      {
        // for each line (= 1 range)
        std::vector<uint8_t> data_line; // 1 line of data
        
        // we first read the gain

        uint8_t byte0 =  *(ping_data.begin() + offset + i * step);
        uint8_t byte1 =  *(ping_data.begin() + offset + i * step +1);
        uint8_t byte2 =  *(ping_data.begin() + offset + i * step +2);
        uint8_t byte3 =  *(ping_data.begin() + offset + i * step +3);
        uint32_t gain = (static_cast<uint32_t>(byte0)) |
                  (static_cast<uint32_t>(byte1) << 8) |
                  (static_cast<uint32_t>(byte2) << 16) |
                  (static_cast<uint32_t>(byte3) << 24);

        // We read the data of the line
        for (int j = SIZE_OF_GAIN_; j < step; j++)
        {
          auto data = *(ping_data.begin() + offset + i * step + j);
          float new_data = data;

          data_line.push_back(new_data);
        }

        // Base gain
        float gain_i = this->gain_applied_;

        // Eventually remove the gain applied by the sonar
        if (this->remove_agc_)
          gain_i/=std::sqrt(static_cast<float>(gain));

        // Compensation of transmission loss

        float r_min = 0.1; // see documentation of the sonar
        float r = r_min + ((r_max-r_min)*((float) i/(float) height));

        // salt/fresh water absorption (same values) 
        float alpha = 0.4; // 0.4-0.5 absorption coefficient, 400dB/km at 1.2 Mhz
        //float alpha = 2.0 - 2.5; // absorption coefficient, 2000-2500 dB/km at 3.0 Mhz

        // Clamp to avoid over-amplification of noise at far ranges, where reflection occurs, S/N is low, and AGC is dominant
        float TL = (r<2.5) ? TL_M3000d_(r, alpha) : TL_M3000d_(2.5, alpha);
        // Convert to linear gains 
        float tvg_factor = pow(10.,(TL/10.));

        if (this->apply_tvg_)
          gain_i *= tvg_factor;  

        // data_line is uint8 array, we first convert it to uint16 array
        size_t n_pixels = ping_msg.n_beams;
        std::vector<uint16_t> data_16(n_pixels);

        for (size_t index = 0; index < n_pixels; index++)
          data_16[index] =
              static_cast<uint16_t>(data_line[2*index]) |
              (static_cast<uint16_t>(data_line[2*index + 1]) << 8); // MONO16 = little-endian

        // Apply the gains on the line (!16-bit!)
        for (size_t index = 0; index < n_pixels; index++)
          {
            float new_data = static_cast<float>(data_16[index]) * gain_i;
            data_16[index] = new_data<65535 ? static_cast<uint16_t>(new_data) : 65535;
          }

        // convert back to uint8


        std::vector<uint8_t> data_8(2 * n_pixels);  // image of (w,h) with 2 bytes per pixel

        for (size_t index = 0; index < n_pixels; index++)
        {
          // pixel i : byte 1
          data_8[2*index]     = data_16[index] & 0xFF;        // Least Significant Byte  (LSB)
          // pixel i : byte 2 (bit shift operation)
          data_8[2*index + 1] = (data_16[index] >> 8) & 0xFF; // Most  Significant  Byte (MSB)
        }
        datas.insert(datas.end(), data_8.begin(), data_8.end());
      }

      rtheta_image.data = datas; 

      // Publish Cartesian image before flipping
      this->publishFan_(ping_msg, rtheta_image); 

      // Flip Polar image vertically 
      if (this->flip_vertically_) {
        std::vector<uint8_t> datas_vflip(datas.size());
        
        // Number of bytes per row (2 bytes per pixel for MONO16)
        int row_size_bytes = width * 2;
        
        // Flip rows: copy rows in reverse order
        for (int row = 0; row < height; row++) {
          int src_row = height - 1 - row;  // Source row (flipped)
          
          // Copy entire row
          for (int byte_idx = 0; byte_idx < row_size_bytes; byte_idx++) {
            datas_vflip[row * row_size_bytes + byte_idx] = 
              datas[src_row * row_size_bytes + byte_idx];
          }
        }
        
        rtheta_image.data = datas_vflip;
      }

      // Publish Polar image
      rtheta_publisher_->publish(rtheta_image);

      // Compress Polar Image and Publish Compressed Polar image
      auto compressed_image = compressImageMsg(rtheta_image);
      rtheta_compressed_publisher_->publish(compressed_image);


}


void SonarViewer::publishFan_(const oculus_interfaces::msg::Ping & ping_msg, 
  const sensor_msgs::msg::Image & image_polar_msg)
{
    // Initialize cartesian image 
    auto fan_image = sensor_msgs::msg::Image();
    fan_image.header = ping_msg.header;
    fan_image.encoding = sensor_msgs::image_encodings::MONO16;

    // Extract polar image parameters
    int n_ranges = ping_msg.n_ranges;
    int n_beams = ping_msg.n_beams;
    double range_max = ping_msg.range;
    double range_resolution = ping_msg.range_resolution;
    
    // Convert bearings from 100th of a degree to radians (see Ping.msg)
    std::vector<double> bearings_rad(n_beams);
    for (int i = 0; i < n_beams; i++) {
        bearings_rad[i] = ping_msg.bearings[i] * 0.01 * M_PI / 180.0;
    }
    
    // Find the bearing extent (min, max)
    double bearing_min = *std::min_element(bearings_rad.begin(), bearings_rad.end());
    double bearing_max = *std::max_element(bearings_rad.begin(), bearings_rad.end());
    
    // Calculate Cartesian image dimensions
    // We need to fit the entire fan sector in a rectangular image
    double r_max = range_max;
    
    // Calculate extreme points of the fan
    double x_min = r_max * std::sin(bearing_min);
    double x_max = r_max * std::sin(bearing_max);
    double y_min = 0.0; // Assuming sonar looks forward
    // taking ymax = r_max as the central beam
    double y_max = r_max; //* std::cos(std::min(std::abs(bearing_min), std::abs(bearing_max))); 
  
    // compute pixels size based on extreme fan points 
    int width = static_cast<int>(std::ceil((x_max - x_min) * this->pixels_per_meter_));
    int height = static_cast<int>(std::ceil((y_max - y_min) * this->pixels_per_meter_));
    
    // Ensure even dimensions (may be required by image codecs)
    if (width % 2 != 0) width++;
    if (height % 2 != 0) height++;
  
    // Update fan (cartesian) image size (w,h) and step
    fan_image.width = width;
    fan_image.height = height;
    fan_image.step = 2 * width; // 2 bytes per pixel for MONO16
    
    // Initialize output image with zeros (all black)
    std::vector<uint16_t> cart_data(width * height, 0);
    
    // Get polar image data as uint16
    const uint16_t* polar_data = reinterpret_cast<const uint16_t*>(image_polar_msg.data.data());
    
    // INVERSE TRANSFORMATION (Polar -> Cartesian)
    // Perform polar to Cartesian transformation
    // For each pixel in the Cartesian image, find corresponding polar coordinates
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Convert pixel coordinates to metric coordinates
            double x_m = x / this->pixels_per_meter_ + x_min;
            double y_m = (height - 1 - y) / this->pixels_per_meter_ + y_min; // Flip y-axis because of image convention
            
            // Convert Cartesian (map) to polar coordinates
            double range = std::sqrt(x_m * x_m + y_m * y_m);
            double bearing = std::atan2(x_m, y_m); // atan2(x, y) for forward-looking sonar, since theta cw wrt y axis
            
            // Check if point is within sonar coverage
            if (range > range_max || range < 0.1 || 
                bearing < bearing_min || bearing > bearing_max) {
                continue; // Leave as zero
            }
            
            // Find range index (direct calculation)
            double range_idx_f = range / range_resolution;
            int range_idx = static_cast<int>(range_idx_f);
            // Check again if point is within sonar coverage
            if (range_idx < 0 || range_idx >= n_ranges - 1) {
                continue;
            }
            
            // Inverse Bearing mapping
            // Find bearing index using binary search or interpolation
            // Since bearings are not uniformly sampled, we need to find nearest
            int beam_idx_lower = 0;
            int beam_idx_upper = n_beams - 1;
            
            // Binary search for the bearing bracket
            // such that current bearing is between two available values
            while (beam_idx_upper - beam_idx_lower > 1) {
                int mid = (beam_idx_lower + beam_idx_upper) / 2;
                if (bearings_rad[mid] < bearing) {
                    beam_idx_lower = mid;
                } else {
                    beam_idx_upper = mid;
                }
            }
            
            // Bilinear interpolation for better quality
            double range_frac = range_idx_f - range_idx;
            double bearing_frac = 0.0;
            
            if (beam_idx_upper > beam_idx_lower && 
                bearings_rad[beam_idx_upper] != bearings_rad[beam_idx_lower]) {
                bearing_frac = (bearing - bearings_rad[beam_idx_lower]) / 
                              (bearings_rad[beam_idx_upper] - bearings_rad[beam_idx_lower]);
            }
            
            // Clamp bearing fraction
            bearing_frac = std::max(0.0, std::min(1.0, bearing_frac));
            
            // Get four neighboring pixels
            uint16_t p00 = polar_data[range_idx * n_beams + beam_idx_lower];
            uint16_t p01 = polar_data[range_idx * n_beams + beam_idx_upper];
            uint16_t p10 = (range_idx + 1 < n_ranges) ? 
                          polar_data[(range_idx + 1) * n_beams + beam_idx_lower] : p00;
            uint16_t p11 = (range_idx + 1 < n_ranges) ? 
                          polar_data[(range_idx + 1) * n_beams + beam_idx_upper] : p01;
            
            // Bilinear interpolation
            double val = (1.0 - range_frac) * ((1.0 - bearing_frac) * p00 + bearing_frac * p01) +
                        range_frac * ((1.0 - bearing_frac) * p10 + bearing_frac * p11);
            
            cart_data[y * width + x] = static_cast<uint16_t>(val);
        }
    }
    
    // Convert to uint8 array for ROS message
    fan_image.data.resize(width * height * 2);                // image of (w,h) with 2 bytes per pixel
    for (size_t i = 0; i < cart_data.size(); i++) {
        // pixel i : byte 1
        fan_image.data[2*i] = cart_data[i] & 0xFF;            // Least Significant Byte  (LSB)
        // pixel i : byte 2 (bit shift operation)
        fan_image.data[2*i + 1] = (cart_data[i] >> 8) & 0xFF; // Most  Significant  Byte (MSB)
    }

    // flip fan image vertically 
    // (!)Not statement since by default the polar image is considered flipped 
    if (!this->flip_vertically_) {
        std::vector<uint8_t> fan_image_vflip(fan_image.data.size());

        // Number of bytes per row (2 bytes per pixel for MONO16)
        int row_size_bytes = width * 2;

        // Flip rows: copy rows in reverse order
        for (int row = 0; row < height; row++) {
          int src_row = height - 1 - row;  // Source row (flipped)
          
          // Copy entire row
          for (int column = 0; column < row_size_bytes; column++) {
            fan_image_vflip[row * row_size_bytes + column] = 
              fan_image.data[src_row * row_size_bytes + column];
          }
        }
        
        fan_image.data = fan_image_vflip;
    }
    
    // Optional: Add grid lines if enabled
    if (this->enable_fan_grid_) {
        addGridLines(fan_image, range_max, bearing_min, bearing_max, 
                    x_min, y_min);
    }
    
    xy_publisher_->publish(fan_image);
}

// TODO: remove it from the ROS image and apply to an OpenCV Image Mat
// I don't need the grid in ROS... Maybe interesting to see in OpenCV
// Grid lines also need to be aware of the coordinate system
void SonarViewer::addGridLines(sensor_msgs::msg::Image& image, 
                                   double range_max,
                                   double bearing_min, 
                                   double bearing_max,
                                   double x_min,
                                   double y_min)
{
    uint16_t* data = reinterpret_cast<uint16_t*>(image.data.data());
    int width = image.width;
    int height = image.height;
    
    uint16_t grid_color = 40000; // Brighter for better visibility
    
    // Determine range interval based on max range
    double range_interval;
    if (range_max <= 5.0) {
        range_interval = 0.5;
    } else if (range_max <= 10.0) {
        range_interval = 1.0;
    } else if (range_max <= 50.0) {
        range_interval = 5.0;
    } else {
        range_interval = 10.0;
    }
    
    // Draw range circles
    for (double r = range_interval; r <= range_max; r += range_interval) {
        for (double theta = bearing_min; theta <= bearing_max; theta += 0.005) {
            double x_m = r * std::sin(theta);
            double y_m = r * std::cos(theta);
            
            int x_pix = static_cast<int>((x_m - x_min) * this->pixels_per_meter_);
            int y_pix = height - 1 - static_cast<int>((y_m - y_min) * this->pixels_per_meter_);
            
            if (x_pix >= 0 && x_pix < width && y_pix >= 0 && y_pix < height) {
                data[y_pix * width + x_pix] = grid_color;
            }
        }
    }
    
    // Draw bearing lines every 10 degrees
    double bearing_interval = 10.0 * M_PI / 180.0;
    for (double theta = std::ceil(bearing_min / bearing_interval) * bearing_interval; 
         theta <= bearing_max; 
         theta += bearing_interval) {
        for (double r = 0.1; r <= range_max; r += 0.05) {
            double x_m = r * std::sin(theta);
            double y_m = r * std::cos(theta);
            
            int x_pix = static_cast<int>((x_m - x_min) * this->pixels_per_meter_);
            int y_pix = height - 1 - static_cast<int>((y_m - y_min) * this->pixels_per_meter_);
            
            if (x_pix >= 0 && x_pix < width && y_pix >= 0 && y_pix < height) {
                data[y_pix * width + x_pix] = grid_color;
            }
        }
    }
}


sensor_msgs::msg::CompressedImage compressImageMsg(const sensor_msgs::msg::Image & image_msg)
{
    // Convert ROS Image message to OpenCV Mat
    cv_bridge::CvImagePtr cv_ptr;
    try {
        cv_ptr = cv_bridge::toCvCopy(image_msg, image_msg.encoding);
    } catch (cv_bridge::Exception& e) {
        throw std::runtime_error("cv_bridge exception: " + std::string(e.what()));
    }

    // Encode image using OpenCV
    std::vector<uchar> compressed_data;
    std::vector<int> compression_params = {cv::IMWRITE_JPEG2000_COMPRESSION_X1000, 100};

    std::string format = "jp2";
    if (!cv::imencode("." + format, cv_ptr->image, compressed_data, compression_params)) {
        throw std::runtime_error("Failed to compress image using OpenCV");
    }

    sensor_msgs::msg::CompressedImage compressed_msg;
    compressed_msg.header = image_msg.header;
    compressed_msg.format = format;
    compressed_msg.data = std::move(compressed_data);

    return compressed_msg;
}

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<SonarViewer>());

    // Possible Multithread Implementation of the ping processing
    // rclcpp::executors::MultiThreadedExecutor executor;
    // auto node = std::make_shared<SonarViewer>();
    // executor.add_node(node);
    // executor.spin();
    
    rclcpp::shutdown();
    return 0;
}