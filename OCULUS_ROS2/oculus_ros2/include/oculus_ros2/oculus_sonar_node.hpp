/**
 * BSD 3-Clause License
 *
 * Copyright (c) 2022, ENSTA-Bretagne
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef OCULUS_ROS2__OCULUS_SONAR_NODE_HPP_
#define OCULUS_ROS2__OCULUS_SONAR_NODE_HPP_

#include <oculus_driver/AsyncService.h>
#include <oculus_driver/SonarDriver.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

#include <oculus_interfaces/msg/oculus_status.hpp>
#include <oculus_interfaces/msg/ping.hpp>
#include <oculus_interfaces/msg/oculus_ping2.hpp>
#include <oculus_ros2/conversions.hpp>
#include <rcl_interfaces/msg/parameter_descriptor.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/fluid_pressure.hpp>
#include <sensor_msgs/msg/temperature.hpp>


/**
 * Snapshot of the user-facing sonar settings.
 * Value-initialised on purpose: every field has a defined value before the first
 * feedback from the sonar arrives.
 */
struct SonarParameters {
  int frequency_mode{0};
  int ping_rate{0};
  int nbeams{0};
  bool gain_assist{false};
  double range{0.0};
  int gamma_correction{0};
  double gain_percent{0.0};
  double sound_speed{0.0};
  bool use_salinity{false};
  double salinity{0.0};
  bool scientific_mode{false};
  bool gain_boost{false};
  bool transmit_boost{false};
};

// Bit masks of PingConfig::flags (the "flags" byte of the fire message).
namespace flagByte {
const int RANGE_AS_METERS = 0x01; // bit 0: 0 = interpret range as percent, 1 = interpret range as meters
const int SIXTEEN_BIT_IMG = 0x02; // bit 1: 0 = 8 bit image, 1 = 16 bit image
// const int ?? = 0x02;           // bit 1: ?? const int DATA_DEPTH = 0x02;  // bit 1: 0 = 8 bit data, 1 = 16 bit data  // inverted ?
const int SEND_GAINS = 0x04;      // bit 2: 0 = won't send gain, 1 = send gain
const int SIMPLE_PING = 0x08;     // bit 3: 0 = send full return message, 1 = send simple return message
const int GAIN_ASSIST = 0x10;     // bit 4: gain assist
// const int ?? = 0x20;           // bit 5: ?
const int NBEAMS = 0x40;          // bit 6: enable 512 beams
// const int ?? = 0x80;           // bit 7: MUST stay 0 (breaks the connection otherwise)
}  // namespace flagByte

/// Bit masks of the extended flags word (OculusSimpleFireMessage2). Not used yet:
/// the PingConfig of the driver version in use has no extFlags field.
namespace extFlagsWord {
// in OculusSimpleFireMessage2 structure 
const int GAIN_REDUCE_6DB = 0x00000001;		    // Not tested
const int GAIN_REDUCE_12DB = 0x00000002;	    // Not tested
const int GAIN_BOOST = 0x00000004;			      // Not tested
const int CHIRP_ENABLE = 0x00000008;		      // Not tested
const int TRANSMIT_BOOST = 0x00000010;		    // Not tested
const int ADVANCED_COMPRESSION = 0x00000020;  // Not tested
const int _4BIT_BMG = 0x00000040;             // overrides flags16BitBmg in flags field		// Not tested
const int UDP_DATA_OUTPUT = 0x00000080;		    // Not tested
const int EXTENDED_TEMPERATURE = 0x00000100;  // Not tested
const int SCIENTIFIC_DATA_MODE = 0x00000200;  // image pixels are 32 bits when this bit is set
} // namespace extFlagWord

/**
 * Declarative description of every dynamic ROS parameter (name, range, default, doc).
 * The constructor loops over these tables to declare the parameters, so adding a
 * parameter starts here (then add it to dynamic_parameters_names_ and handle it in
 * updateLocalParameters / sendParamToSonar).
 */
namespace params {

const double TEMPERATURE_WARN_DEFAULT_VALUE = 30.;
const double TEMPERATURE_STOP_DEFAULT_VALUE = 35.;
const bool RUN_MODE_DEFAULT_VALUE = true;

struct BoolParam {
  const std::string name;
  const bool default_val;
  const std::string desc;
};

// NOTE: the identifier keeps its historical spelling (GAIN_ASSIT) so other files still compile.
// Default is false: gain assist is forced off at frequency_mode 2 (the default mode) anyway.
const BoolParam GAIN_ASSIT = {"gain_assist", false, "Gain assist (only usable in frequency_mode 1, currently unreliable)."};
const BoolParam USE_SALINITY = {"use_salinity", true, "Let the sonar compute the sound speed from salinity (sound_speed is then ignored)."};

// Not implemented yet in this driver: setting them to true is rejected.
const BoolParam SWITCH_SCIENTIFIC_DATA_MODE = {"switch_scientific_data_mode",false,"NOT IMPLEMENTED. Image pixels are 32 bits when set"};
const BoolParam SET_GAIN_BOOST = {"set_gain_boost",false,"NOT IMPLEMENTED. Might be rx gain boost."};
const BoolParam SET_TRANSMIT_BOOST = {"set_transmit_boost",false,"NOT IMPLEMENTED. Boost transmit signal."};

const std::vector<BoolParam> BOOL = {GAIN_ASSIT, USE_SALINITY,SWITCH_SCIENTIFIC_DATA_MODE,SET_GAIN_BOOST,SET_TRANSMIT_BOOST};

struct IntParam {
  const std::string name;
  const int min;
  const int max;
  const int default_val;
  const std::string desc;
};

const IntParam FREQUENCY_MODE = {
    "frequency_mode", 1, 2, 2,
    "Sonar beam frequency mode.\n"
    "\t1: Low frequency  (1.2MHz, wide aperture).\n"
    "\t2: High frequency (3.0Mhz, narrow aperture)."
};

const IntParam PING_RATE = {
    "ping_rate", 0, 5, 0,
    "Frequency of ping fires.\n\t" + std::to_string(pingRateNormal) + ": 10Hz max ping rate.\n\t" + std::to_string(pingRateHigh) +
        ": 15Hz max ping rate.\n\t" + std::to_string(pingRateHighest) + ": 40Hz max ping rate.\n\t" +
        std::to_string(pingRateLow) + ": 5Hz max ping rate.\n\t" + std::to_string(pingRateLowest) + ": 2Hz max ping rate.\n\t" +
        std::to_string(pingRateStandby) + ": Standby mode (no ping fire)"
};

const IntParam NBEAMS = {
    "nbeams", 0, 1, 1,
    "Number of ping beams.\n"
    "\t0: Oculus outputs 256 beams.\n"
    "\t1: Oculus outputs 512 beams."
};

const IntParam GAMMA_CORRECTION = {"gamma_correction", 1, 255, 153, "Gamma correction, min=1, max=255."};

const std::vector<IntParam> INT = {FREQUENCY_MODE, PING_RATE, NBEAMS, GAMMA_CORRECTION};

struct DoubleParam {
  const std::string name;
  const double min;
  const double max;
  const double step; // informational only: the ROS range is declared continuous (step 0)
  const double default_val;
  const std::string desc;
};

const DoubleParam RANGE = {"range", .3, 30., .1, 5., "Sonar range (in meters), min=0.3, max=30.0."};
const DoubleParam GAIN_PERCENT = {"gain_percent", .1, 100., .1, 50., "Gain percentage (%), min=0.1, max=100.0."};
const DoubleParam SOUND_SPEED = {"sound_speed", 0., 1600., .1, 0.,
    "Sound speed (in m/s). 0 = computed by the sonar from salinity, otherwise 1400.0 to 1600.0. "
    "Values other than 0 are ignored while use_salinity is true."};
const DoubleParam SALINITY = {"salinity", 0., 100., .1, 0.,
    "Salinity (in parts per thousand (ppt,ppm,g/kg), "
     "used to calculate sound speed when use_salinity is true), min=0.0, max=100"};

const std::vector<DoubleParam> DOUBLE = {RANGE, GAIN_PERCENT, SOUND_SPEED, SALINITY};

}  // namespace params

namespace detail {
 
// Two doubles closer than this are considered equal when comparing what we asked
// the sonar for with what it reports back (half of the 0.1 resolution of the tables above).
// Tune it to the real resolution of the sonar if you see spurious mismatch warnings.
constexpr double FLOAT_TOLERANCE = 0.05;
 
// "Different" test that is safe for doubles (tolerance) and exact for int / bool.
template <class T>
bool differs(const T& a, const T& b) {
  if constexpr (std::is_floating_point_v<T>) {
    return std::fabs(a - b) > FLOAT_TOLERANCE;
  } else {
    return a != b;
  }
}
 
}  // namespace detail

/**
 * OculusSonarNode: ROS 2 wrapper around the oculus_driver SonarDriver (M3000d).
 *
 * THREADS
 *  - ROS executor thread (single-threaded spin): constructor, parameter callbacks
 *    (setConfigCallback) and the sync timer (syncRosParameters).
 *  - Driver io thread: status / ping / dummy callbacks (publishStatus, publishPing, handleDummy).
 *  The io thread NEVER touches ROS parameters. It only updates atomics and
 *  currentSonarParameters_ (under state_mutex_). All ROS parameter writes coming from the
 *  sonar happen on the executor thread through the sync timer. This avoids both data races
 *  and a possible deadlock (the executor blocks in request_ping_config() waiting for the io
 *  thread, so the io thread must never wait for something the executor holds).
 *
 * STATE (three views of the configuration, kept consistent on purpose)
 *  - currentConfig_            last PingConfig confirmed by the sonar. Base of every new request.
 *  - currentSonarParameters_   what the sonar really uses (feedback + pings). Guarded by state_mutex_.
 *  - currentRosParameters_     last values accepted by / written to the ROS parameter server.
 *                              Only touched on the executor thread (no lock needed).
 *
 * ROS -> SONAR : setConfigCallback validates ONE parameter, sendParamToSonar sends
 *                currentConfig_ + that change (forced flags via setMinimalFlags), compares the
 *                sonar feedback with the request and rejects the parameter if it was not honoured.
 * SONAR -> ROS : pings refresh currentSonarParameters_; syncRosParameters (10 Hz timer) overwrites
 *                any ROS parameter that differs (the sonar is the source of truth).
 * RUN / STANDBY: "run" = user intent (is_running_). Standby is forced by overheating or
 *                ping_rate == standby; handleDummy resumes the sonar when there are subscribers.
 */

class OculusSonarNode : public rclcpp::Node {
public:
  OculusSonarNode();
  ~OculusSonarNode();

protected:
  // Parameters replayed through setConfigCallback at startup, in this order.
  // gain_assist comes first so that frequency_mode is not blocked by the gain-assist guard.
  const std::vector<std::string> dynamic_parameters_names_{params::GAIN_ASSIT.name, params::FREQUENCY_MODE.name,
      params::PING_RATE.name, params::NBEAMS.name, params::RANGE.name, params::GAMMA_CORRECTION.name,
      params::GAIN_PERCENT.name, params::SOUND_SPEED.name, params::USE_SALINITY.name, params::SALINITY.name,
      params::SWITCH_SCIENTIFIC_DATA_MODE.name, params::SET_GAIN_BOOST.name, params::SET_TRANSMIT_BOOST.name, "run"};

  SonarParameters currentSonarParameters_{};        // guarded by state_mutex_
  SonarParameters currentRosParameters_{};          // executor thread only
  oculus::SonarDriver::PingConfig currentConfig_;   // executor thread only
 
  std::atomic<bool> is_running_;  // User intent. Mirrors the ROS parameter "run" (see syncRosParameters)
  std::atomic<bool> is_overheating_{false};

  // Protects currentSonarParameters_ ONLY. Always held briefly, never across a call to the sonar.
  mutable std::mutex state_mutex_;

  // Steady-clock time (ns) of the last config answer from the sonar. Used to ignore pings
  // that were already in flight with the old settings (see publishPing).
  std::atomic<std::int64_t> last_config_answer_ns_{0};
 
  int get_subscription_count() const;

private:
  std::shared_ptr<oculus::SonarDriver> sonar_driver_;
  oculus::AsyncService io_service_;
  // rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr image_publisher_;
  const std::string frame_id_;
  const double temperature_warn_limit_;
  const double temperature_stop_limit_;
  rclcpp::Publisher<oculus_interfaces::msg::OculusStatus>::SharedPtr status_publisher_{nullptr};
  rclcpp::Publisher<oculus_interfaces::msg::Ping>::SharedPtr ping_publisher_{nullptr};
  rclcpp::Publisher<oculus_interfaces::msg::OculusPing2>::SharedPtr oculus_ping_publisher_{nullptr};
  rclcpp::Publisher<sensor_msgs::msg::Temperature>::SharedPtr temperature_publisher_{nullptr};
  rclcpp::Publisher<sensor_msgs::msg::FluidPressure>::SharedPtr pressure_publisher_{nullptr};

  rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr param_cb_{nullptr};
  rclcpp::TimerBase::SharedPtr sync_timer_{nullptr};

  // When true, setConfigCallback accepts everything without touching the sonar.
  // Set (on the calling thread only) while the node itself writes a parameter to mirror the sonar state.
  static thread_local bool syncing_from_driver_;

  // ROS - sonar synchronization 
  template <class T>
  void updateRosConfigForParam(T& ros_mirror, const T& sonar_value, const std::string& param_name);
  void syncRosParameters();
  bool setParameterBypassingValidation(const rclcpp::Parameter& param);

  template <class T>
  void handleFeedbackForParam(rcl_interfaces::msg::SetParametersResult& result,
      const rclcpp::Parameter& param,
      const T& old_val,
      const T& new_val,
      const std::string& param_name) const;

  SonarParameters sonarParametersSnapshot() const;
  void updateLocalParameters(SonarParameters& parameters, const std::vector<rclcpp::Parameter>& new_parameters);
  void updateLocalParameters(SonarParameters& parameters, oculus::SonarDriver::PingConfig feedback);
  void sendParamToSonar(const rclcpp::Parameter& param, rcl_interfaces::msg::SetParametersResult& result);
  rcl_interfaces::msg::SetParametersResult setConfigCallback(const std::vector<rclcpp::Parameter>& parameters);

  // run/standby
  void enableRunMode();
  void disableRunMode();
  void checkOverheating(const double& new_temperature);

  // flags
  void setMinimalFlags(uint8_t& flags, int master_mode) const;
  void checkMinimalFlags(const uint8_t& flags) const;

  // driver callbacks (io thread)
  void publishStatus(const OculusStatusMsg& status);
  void publishPing(const oculus::PingMessage::ConstPtr& pingMetadata);
  void handleDummy();
};

/**
 * Sonar -> ROS for one parameter. If the value reported by the sonar differs from what the
 * ROS parameter server holds, overwrite the ROS parameter (executor thread only).
 */
template <class T>
void OculusSonarNode::updateRosConfigForParam(T& ros_mirror, const T& sonar_value, const std::string& param_name) {
  if (!detail::differs(ros_mirror, sonar_value)) {
    return;
  }
  RCLCPP_WARN_STREAM(this->get_logger(),
      "The parameter " << param_name << " has changed by itself from " << ros_mirror << " to " << sonar_value);
  ros_mirror = sonar_value;
  this->setParameterBypassingValidation(rclcpp::Parameter(param_name, sonar_value));
}

/**
 * Compare what we requested (old_val) with what the sonar answered (new_val).
 *  - If the mismatching field IS the parameter being set: reject the set.
 *  - Otherwise the sonar changed something else as a side effect: only warn.
 * `result` is a reference: the verdict must reach setConfigCallback.
 */
template <class T>
void OculusSonarNode::handleFeedbackForParam(rcl_interfaces::msg::SetParametersResult& result,
    const rclcpp::Parameter& param,
    const T& old_val,
    const T& new_val,
    const std::string& param_name) const {
  if (detail::differs(old_val, new_val)) {
    if (param.get_name() == param_name) {
      result.successful = false;
      RCLCPP_WARN_STREAM(this->get_logger(),
          "Could not update " << param_name << " (asked " << old_val << ", sonar answered " << new_val << ")");
      result.reason.append("Could not update " + param_name + ".\n");
    } else {
      RCLCPP_WARN_STREAM(this->get_logger(),
          param_name << " changed from " << old_val << " to " << new_val << " when updating the parameter " << param.get_name());
      result.reason.append(param_name + " changed.\n");
    }
  }
}

#endif  // OCULUS_ROS2__OCULUS_SONAR_NODE_HPP_
