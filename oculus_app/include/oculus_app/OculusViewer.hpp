
#ifndef OCULUS_VIEWER_HPP
#define OCULUS_VIEWER_HPP

// OpenCV
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

// Oculus Driver
#include <oculus_driver/Oculus.h>
#include <oculus_driver/OculusMessage.h>

// Logger
#include <oculus_app/Logger.hpp>

// Utilities
#include <oculus_app/sonar_utils.hpp>

#include <memory>
#include <string>

class OculusViewer {
public:
    OculusViewer(std::shared_ptr<Logger> logger = nullptr);
    ~OculusViewer();

    /**
     * @brief Process and display ping data
     * @param ping Ping message from sonar
     */
    void displayPing(const oculus::PingMessage::ConstPtr& ping);

    /**
     * @brief Set viewer parameters
     */
    void setRemoveAGC(bool remove) { remove_agc_ = remove; }
    void setApplyTVG(bool apply) { apply_tvg_ = apply; }
    void setGain(double gain) { gain_applied_ = gain; }
    void setPixelsPerMeter(double ppm) { pixels_per_meter_ = ppm; }
    void setEnableGrid(bool enable) { enable_fan_grid_ = enable; }
    void setFlipVertically(bool flip) { flip_vertically_ = flip; }

    /**
     * @brief Get viewer parameters
     */
    bool getRemoveAGC() const { return remove_agc_; }
    bool getApplyTVG() const { return apply_tvg_; }
    double getGain() const { return gain_applied_; }
    double getPixelsPerMeter() const { return pixels_per_meter_; }
    bool getEnableGrid() const { return enable_fan_grid_; }
    bool getFlipVertically() const { return flip_vertically_; }

private:
    std::shared_ptr<Logger> console_log_;

    // Viewer parameters
    bool remove_agc_;
    bool apply_tvg_;
    double gain_applied_;
    double pixels_per_meter_;
    bool enable_fan_grid_;
    bool flip_vertically_;

    // Window names
    const std::string window_polar_ = "Oculus Sonar - Polar View";
    const std::string window_cartesian_ = "Oculus Sonar - Cartesian View";

    /**
     * @brief Create polar image from ping data
     */
    cv::Mat createPolarImage(const oculus::PingMessage::ConstPtr& ping);

    /**
     * @brief Create Cartesian image from polar image
     */
    cv::Mat createCartesianImage(const cv::Mat& polar_image,
                                  const oculus::PingMessage::ConstPtr& ping);
};

#endif // OCULUS_VIEWER_HPP