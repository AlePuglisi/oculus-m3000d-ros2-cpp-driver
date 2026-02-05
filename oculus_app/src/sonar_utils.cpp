#include "oculus_app/sonar_utils.hpp"
#include <algorithm>

namespace sonar_utils {

std::vector<uint16_t> processPingData(
    const std::vector<uint8_t>& raw_data,
    int n_ranges,
    int n_beams,
    double range_max,
    double gain_applied,
    bool remove_agc,
    bool apply_tvg,
    float alpha)
{
    const int SIZE_OF_GAIN = 4;
    const int step = 2 * n_beams + SIZE_OF_GAIN;
    const int offset = 2048; // UDP packet offset

    std::vector<uint16_t> processed_data(n_ranges * n_beams);

    for (int i = 0; i < n_ranges; ++i) {
        // Read gain for this range line
        uint8_t byte0 = raw_data[offset + i * step];
        uint8_t byte1 = raw_data[offset + i * step + 1];
        uint8_t byte2 = raw_data[offset + i * step + 2];
        uint8_t byte3 = raw_data[offset + i * step + 3];
        
        uint32_t gain = (static_cast<uint32_t>(byte0)) |
                       (static_cast<uint32_t>(byte1) << 8) |
                       (static_cast<uint32_t>(byte2) << 16) |
                       (static_cast<uint32_t>(byte3) << 24);

        // Calculate gain for this line
        float gain_i = gain_applied;

        // Remove AGC if enabled
        if (remove_agc) {
            gain_i /= std::sqrt(static_cast<float>(gain));
        }

        // Apply TVG if enabled
        if (apply_tvg) {
            float r_min = 0.1; // minimum range
            float r = r_min + ((range_max - r_min) * (static_cast<float>(i) / n_ranges));
            
            // Clamp to avoid over-amplification at far ranges
            float TL = (r < 2.5) ? computeTransmissionLoss(r, alpha) 
                                 : computeTransmissionLoss(2.5, alpha);
            float tvg_factor = std::pow(10.0f, TL / 10.0f);
            gain_i *= tvg_factor;
        }

        // Process each beam in this range line
        for (int j = 0; j < n_beams; j++) {
            int data_idx = offset + i * step + SIZE_OF_GAIN + 2 * j;
            
            // Read 16-bit data (little-endian)
            uint16_t data_16 = static_cast<uint16_t>(raw_data[data_idx]) |
                              (static_cast<uint16_t>(raw_data[data_idx + 1]) << 8);

            // Apply gain
            float new_data = static_cast<float>(data_16) * gain_i;
            processed_data[i * n_beams + j] = (new_data < 65535) 
                                              ? static_cast<uint16_t>(new_data) 
                                              : 65535;
        }
    }

    return processed_data;
}

cv::Mat polarToCartesian(
    const std::vector<uint16_t>& polar_data,
    int n_ranges,
    int n_beams,
    const std::vector<double>& bearings,
    double range_max,
    double range_resolution,
    double pixels_per_meter)
{
    // Find bearing extent
    double bearing_min = *std::min_element(bearings.begin(), bearings.end());
    double bearing_max = *std::max_element(bearings.begin(), bearings.end());

    // Calculate Cartesian image dimensions
    double r_max = range_max;
    double x_min = r_max * std::sin(bearing_min);
    double x_max = r_max * std::sin(bearing_max);
    double y_min = 0.0;
    double y_max = r_max;

    int width = static_cast<int>(std::ceil((x_max - x_min) * pixels_per_meter));
    int height = static_cast<int>(std::ceil((y_max - y_min) * pixels_per_meter));

    // Ensure even dimensions
    if (width % 2 != 0) width++;
    if (height % 2 != 0) height++;

    // Create output image
    cv::Mat cart_image = cv::Mat::zeros(height, width, CV_16UC1);

    // Perform polar to Cartesian transformation
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Convert pixel to metric coordinates
            double x_m = x / pixels_per_meter + x_min;
            double y_m = (height - 1 - y) / pixels_per_meter + y_min;

            // Convert to polar coordinates
            double range = std::sqrt(x_m * x_m + y_m * y_m);
            double bearing = std::atan2(x_m, y_m);

            // Check if within sonar coverage
            if (range > range_max || range < 0.1 ||
                bearing < bearing_min || bearing > bearing_max) {
                continue;
            }

            // Find range index
            double range_idx_f = range / range_resolution;
            int range_idx = static_cast<int>(range_idx_f);
            
            if (range_idx < 0 || range_idx >= n_ranges - 1) {
                continue;
            }

            // Find bearing indices using binary search
            int beam_idx_lower = 0;
            int beam_idx_upper = n_beams - 1;

            while (beam_idx_upper - beam_idx_lower > 1) {
                int mid = (beam_idx_lower + beam_idx_upper) / 2;
                if (bearings[mid] < bearing) {
                    beam_idx_lower = mid;
                } else {
                    beam_idx_upper = mid;
                }
            }

            // Bilinear interpolation
            double range_frac = range_idx_f - range_idx;
            double bearing_frac = 0.0;

            if (beam_idx_upper > beam_idx_lower &&
                bearings[beam_idx_upper] != bearings[beam_idx_lower]) {
                bearing_frac = (bearing - bearings[beam_idx_lower]) /
                              (bearings[beam_idx_upper] - bearings[beam_idx_lower]);
            }

            bearing_frac = std::max(0.0, std::min(1.0, bearing_frac));

            // Get four neighboring pixels
            uint16_t p00 = polar_data[range_idx * n_beams + beam_idx_lower];
            uint16_t p01 = polar_data[range_idx * n_beams + beam_idx_upper];
            uint16_t p10 = (range_idx + 1 < n_ranges) ?
                          polar_data[(range_idx + 1) * n_beams + beam_idx_lower] : p00;
            uint16_t p11 = (range_idx + 1 < n_ranges) ?
                          polar_data[(range_idx + 1) * n_beams + beam_idx_upper] : p01;

            // Interpolate
            double val = (1.0 - range_frac) * ((1.0 - bearing_frac) * p00 + bearing_frac * p01) +
                        range_frac * ((1.0 - bearing_frac) * p10 + bearing_frac * p11);

            cart_image.at<uint16_t>(y, x) = static_cast<uint16_t>(val);
        }
    }

    return cart_image;
}

void addGridLines(
    cv::Mat& image,
    double range_max,
    double bearing_min,
    double bearing_max,
    double x_min,
    double y_min,
    double pixels_per_meter,
    uint16_t grid_color)
{
    int width = image.cols;
    int height = image.rows;

    // Determine range interval
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

            int x_pix = static_cast<int>((x_m - x_min) * pixels_per_meter);
            int y_pix = height - 1 - static_cast<int>((y_m - y_min) * pixels_per_meter);

            if (x_pix >= 0 && x_pix < width && y_pix >= 0 && y_pix < height) {
                image.at<uint16_t>(y_pix, x_pix) = grid_color;
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

            int x_pix = static_cast<int>((x_m - x_min) * pixels_per_meter);
            int y_pix = height - 1 - static_cast<int>((y_m - y_min) * pixels_per_meter);

            if (x_pix >= 0 && x_pix < width && y_pix >= 0 && y_pix < height) {
                image.at<uint16_t>(y_pix, x_pix) = grid_color;
            }
        }
    }
}

cv::Mat convert16to8bit(const cv::Mat& image_16) {
    cv::Mat image_8;
    image_16.convertTo(image_8, CV_8UC1, 255.0 / 65535.0);
    return image_8;
}

cv::Mat flipVertically(const cv::Mat& image) {
    cv::Mat flipped;
    cv::flip(image, flipped, 0);  // 0 = flip around x-axis
    return flipped;
}

} // namespace sonar_utils