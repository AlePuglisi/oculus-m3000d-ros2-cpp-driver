#include "oculus_app/OculusViewer.hpp"

OculusViewer::OculusViewer(std::shared_ptr<Logger> logger)
    : console_log_(logger),
      remove_agc_(true),
      apply_tvg_(true),
      gain_applied_(3000.0),
      pixels_per_meter_(100.0),
      enable_fan_grid_(true),
      flip_vertically_(true)
{
    if (!console_log_) {
        console_log_ = std::make_shared<Logger>();
    }

    console_log_->log(INFO, "Initializing OculusViewer...");

    // Create OpenCV windows
    cv::namedWindow(window_polar_, cv::WINDOW_NORMAL);
    cv::namedWindow(window_cartesian_, cv::WINDOW_NORMAL);

    console_log_->log(INFO, "Viewer Parameters:");
    console_log_->log(INFO, "  Remove AGC: " + std::string(remove_agc_ ? "true" : "false"));
    console_log_->log(INFO, "  Apply TVG: " + std::string(apply_tvg_ ? "true" : "false"));
    console_log_->log(INFO, "  Gain: " + std::to_string(gain_applied_));
    console_log_->log(INFO, "  Pixels per meter: " + std::to_string(pixels_per_meter_));
    console_log_->log(INFO, "  Enable grid: " + std::string(enable_fan_grid_ ? "true" : "false"));
    console_log_->log(INFO, "  Flip vertically: " + std::string(flip_vertically_ ? "true" : "false"));

    if (apply_tvg_ && !remove_agc_) {
        console_log_->log(WARNING, "TVG without AGC removal may not be effective");
    }

    console_log_->log(INFO, "OculusViewer Initialized");
}

OculusViewer::~OculusViewer()
{
    console_log_->log(INFO, "Shutting down OculusViewer...");
    cv::destroyWindow(window_polar_);
    cv::destroyWindow(window_cartesian_);
    console_log_->log(INFO, "OculusViewer stopped");
}

void OculusViewer::displayPing(const oculus::PingMessage::ConstPtr& ping)
{
    try {
        // Create polar image
        cv::Mat polar_image = createPolarImage(ping);

        if (polar_image.empty()) {
            console_log_->log(WARNING, "Failed to create polar image");
            return;
        }

        // Create Cartesian image
        cv::Mat cartesian_image = createCartesianImage(polar_image, ping);

        // Convert to 8-bit for display
        cv::Mat polar_display = sonar_utils::convert16to8bit(polar_image);
        cv::Mat cart_display = sonar_utils::convert16to8bit(cartesian_image);

        // Display images
        cv::imshow(window_polar_, polar_display);
        cv::imshow(window_cartesian_, cart_display);
        cv::waitKey(1); // Process GUI events

    } catch (const std::exception& e) {
        console_log_->log(ERROR, "Error displaying ping: " + std::string(e.what()));
    }
}

cv::Mat OculusViewer::createPolarImage(const oculus::PingMessage::ConstPtr& ping)
{
    int n_ranges = ping->range_count();
    int n_beams = ping->bearing_count();
    double range_max = ping->range();

    // Get raw ping data
    const std::vector<uint8_t>& raw_data = ping->data();

    // Determine absorption coefficient based on frequency
    float alpha = (ping->master_mode() == 1) ? 0.4f : 2.2f;  // 1.2MHz vs 3.0MHz

    // Process ping data
    std::vector<uint16_t> processed_data = sonar_utils::processPingData(
        raw_data,
        n_ranges,
        n_beams,
        range_max,
        gain_applied_,
        remove_agc_,
        apply_tvg_,
        alpha);

    // Create OpenCV Mat
    cv::Mat polar_image(n_ranges, n_beams, CV_16UC1, processed_data.data());
    
    // Clone to own the data
    polar_image = polar_image.clone();

    // Flip vertically if requested
    if (flip_vertically_) {
        polar_image = sonar_utils::flipVertically(polar_image);
    }

    return polar_image;
}

cv::Mat OculusViewer::createCartesianImage(const cv::Mat& polar_image,
                                            const oculus::PingMessage::ConstPtr& ping)
{
    int n_ranges = ping->range_count();
    int n_beams = ping->bearing_count();
    double range_max = ping->range();
    double range_resolution = ping->range_resolution();

    // Convert bearings from 100th of a degree to radians
    std::vector<double> bearings_rad(n_beams);
    const int16_t* bearing_data = ping->bearing_data();
    
    for (int i = 0; i < n_beams; i++) {
        bearings_rad[i] = bearing_data[i] * 0.01 * M_PI / 180.0;
    }

    // Convert polar image data to vector
    std::vector<uint16_t> polar_data(n_ranges * n_beams);
    for (int i = 0; i < n_ranges; i++) {
        for (int j = 0; j < n_beams; j++) {
            polar_data[i * n_beams + j] = polar_image.at<uint16_t>(i, j);
        }
    }

    // Perform polar to Cartesian conversion
    cv::Mat cart_image = sonar_utils::polarToCartesian(
        polar_data,
        n_ranges,
        n_beams,
        bearings_rad,
        range_max,
        range_resolution,
        pixels_per_meter_);

    // Add grid if enabled
    if (enable_fan_grid_) {
        double bearing_min = *std::min_element(bearings_rad.begin(), bearings_rad.end());
        double bearing_max = *std::max_element(bearings_rad.begin(), bearings_rad.end());
        
        double x_min = range_max * std::sin(bearing_min);
        double y_min = 0.0;

        sonar_utils::addGridLines(
            cart_image,
            range_max,
            bearing_min,
            bearing_max,
            x_min,
            y_min,
            pixels_per_meter_);
    }

    // Flip if not already flipped in polar domain
    if (!flip_vertically_) {
        cart_image = sonar_utils::flipVertically(cart_image);
    }

    return cart_image;
}