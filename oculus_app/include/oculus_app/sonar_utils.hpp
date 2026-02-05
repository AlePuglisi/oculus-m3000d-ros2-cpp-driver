/**
 * BSD 3-Clause License
 *
 * Copyright (c) 2022, ENSTA-Bretagne
 * All rights reserved.
 */

#ifndef SONAR_UTILS_HPP
#define SONAR_UTILS_HPP

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <vector>
#include <cmath>

namespace sonar_utils {

/**
 * @brief Structure to hold ping data in a convenient format
 */
struct PingData {
    int n_ranges;
    int n_beams;
    double range;
    double range_resolution;
    std::vector<double> bearings;  // in radians
    std::vector<uint16_t> data;    // processed image data (n_ranges x n_beams)
};

/**
 * @brief Compute Transmission Loss for M3000d sonar
 * @param range Range of interest [m]
 * @param alpha Absorption coefficient (0.3-0.4 at 1.2MHz, 2.0-2.5 at 3.0MHz) [dB/m]
 * @return Transmission loss in dB
 */
inline float computeTransmissionLoss(float range, float alpha = 0.3) {
    // Geometrical divergence (10log10) + absorption (2*alpha for round trip)
    return 10.0f * std::log10(range) + 2.0f * alpha * range;
}

/**
 * @brief Process raw ping data with AGC removal and TVG application
 * @param raw_data Raw ping data from sonar
 * @param n_ranges Number of range bins
 * @param n_beams Number of beams
 * @param range_max Maximum range [m]
 * @param gain_applied Base gain to apply
 * @param remove_agc Remove automatic gain control
 * @param apply_tvg Apply time-variable gain
 * @param alpha Absorption coefficient
 * @return Processed 16-bit image data
 */
std::vector<uint16_t> processPingData(
    const std::vector<uint8_t>& raw_data,
    int n_ranges,
    int n_beams,
    double range_max,
    double gain_applied,
    bool remove_agc,
    bool apply_tvg,
    float alpha = 0.4);

/**
 * @brief Convert polar image to Cartesian (fan) representation
 * @param polar_data Polar image data (n_ranges x n_beams)
 * @param n_ranges Number of range bins
 * @param n_beams Number of beams
 * @param bearings Bearing angles in radians
 * @param range_max Maximum range [m]
 * @param range_resolution Range resolution [m]
 * @param pixels_per_meter Resolution in pixels/meter
 * @return OpenCV Mat in Cartesian coordinates (CV_16UC1)
 */
cv::Mat polarToCartesian(
    const std::vector<uint16_t>& polar_data,
    int n_ranges,
    int n_beams,
    const std::vector<double>& bearings,
    double range_max,
    double range_resolution,
    double pixels_per_meter);

/**
 * @brief Add grid lines to Cartesian image
 * @param image Image to add grid to (modified in place)
 * @param range_max Maximum range [m]
 * @param bearing_min Minimum bearing [rad]
 * @param bearing_max Maximum bearing [rad]
 * @param x_min X offset [m]
 * @param y_min Y offset [m]
 * @param pixels_per_meter Resolution in pixels/meter
 * @param grid_color Color value for grid lines
 */
void addGridLines(
    cv::Mat& image,
    double range_max,
    double bearing_min,
    double bearing_max,
    double x_min,
    double y_min,
    double pixels_per_meter,
    uint16_t grid_color = 40000);

/**
 * @brief Convert 16-bit image to 8-bit for display
 * @param image_16 Input 16-bit image
 * @return 8-bit image (CV_8UC1)
 */
cv::Mat convert16to8bit(const cv::Mat& image_16);

/**
 * @brief Flip image vertically
 * @param image Input image
 * @return Flipped image
 */
cv::Mat flipVertically(const cv::Mat& image);

} // namespace sonar_utils

#endif // SONAR_UTILS_HPP