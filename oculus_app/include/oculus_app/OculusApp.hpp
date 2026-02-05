#ifndef OCULUS_APP_HPP
#define OCULUS_APP_HPP

// Oculus Driver 
#include <oculus_driver/AsyncService.h>
#include <oculus_driver/SonarDriver.h>
// OpenCV 
#include <opencv2/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
// Logger
#include <oculus_app/Logger.hpp>
// Viewer
#include <oculus_app/OculusViewer.hpp>

// Utils Liobrary
#include <iostream>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

struct SonarParameters {
  int frequency_mode;
  int ping_rate;
  int nbeams;
  bool gain_assist;
  double range;
  int gamma_correction;
  double gain_percent;
  double sound_speed;
  bool use_salinity;
  double salinity;
  bool scientific_mode;
  bool gain_boost;
  bool transmit_boost;

  // Equality operator for easy comparison
  bool operator==(const SonarParameters& other) const {
    return frequency_mode == other.frequency_mode &&
           ping_rate == other.ping_rate &&
           nbeams == other.nbeams &&
           gain_assist == other.gain_assist &&
           std::abs(range - other.range) < 0.01 &&
           gamma_correction == other.gamma_correction &&
           std::abs(gain_percent - other.gain_percent) < 0.01 &&
           std::abs(sound_speed - other.sound_speed) < 0.01 &&
           use_salinity == other.use_salinity &&
           std::abs(salinity - other.salinity) < 0.01 &&
           scientific_mode == other.scientific_mode &&
           gain_boost == other.gain_boost &&
           transmit_boost == other.transmit_boost;
  }

  bool operator!=(const SonarParameters& other) const {
    return !(*this == other);
  }
};

namespace flagByte {
const int RANGE_AS_METERS = 0x01;  // bit 0: 0 = interpret range as percent, 1 = interpret range as meters
const int SIXTEEN_BIT_IMG = 0x02;  // bit 1: 0 = 8 bit image, 1 = 16 bit image
const int SEND_GAINS = 0x04;  // bit 2: 0 = won't send gain, 1 = send gain
const int SIMPLE_PING = 0x08;  // bit 3: 0 = send full return message, 1 = send simple return message
const int GAIN_ASSIST = 0x10;  // bit 4: gain assist
const int NBEAMS = 0x40;  // bit 6: enable 512 beams
}  // namespace flagByte

namespace extFlagsWord {
const int GAIN_REDUCE_6DB = 0x00000001;
const int GAIN_REDUCE_12DB = 0x00000002;
const int GAIN_BOOST = 0x00000004;
const int CHIRP_ENABLE = 0x00000008;
const int TRANSMIT_BOOST = 0x00000010;
const int ADVANCED_COMPRESSION = 0x00000020;
const int _4BIT_BMG = 0x00000040;
const int UDP_DATA_OUTPUT = 0x00000080;
const int EXTENDED_TEMPERATURE = 0x00000100;
const int SCIENTIFIC_DATA_MODE = 0x00000200;
} // namespace extFlagsWord

namespace params {

const double TEMPERATURE_WARN_DEFAULT_VALUE = 30.;
const double TEMPERATURE_STOP_DEFAULT_VALUE = 35.;
const bool RUN_MODE_DEFAULT_VALUE = true;

struct BoolParam {
  const std::string name;
  const bool default_val;
  const std::string desc;
};

const BoolParam GAIN_ASSIT = {"gain_assist", true, "Enable automatic gain adjustment"};
const BoolParam USE_SALINITY = {"use_salinity", true, "Use salinity to calculate sound_speed."};
const BoolParam SWITCH_SCIENTIFIC_DATA_MODE = {"switch_scientific_data_mode", false, "Image pixels are 32 bits when set"};
const BoolParam SET_GAIN_BOOST = {"set_gain_boost", false, "Enable RX gain boost"};
const BoolParam SET_TRANSMIT_BOOST = {"set_transmit_boost", false, "Boost transmit signal"};

const std::vector<BoolParam> BOOL = {GAIN_ASSIT, USE_SALINITY, SWITCH_SCIENTIFIC_DATA_MODE, SET_GAIN_BOOST, SET_TRANSMIT_BOOST};

struct IntParam {
  const std::string name;
  const int min;
  const int max;
  const int default_val;
  const std::string desc;
};

const IntParam FREQUENCY_MODE = {"frequency_mode", 1, 2, 2,
    "Sonar beam frequency mode.\n"
    "\t1: Low frequency  (1.2MHz, wide aperture).\n"
    "\t2: High frequency (3.0Mhz, narrow aperture)."};

const IntParam PING_RATE = {"ping_rate", 0, 5, 0,
    "Frequency of ping fires.\n\t" + std::to_string(pingRateNormal) + ": 10Hz max ping rate.\n\t" + std::to_string(pingRateHigh) +
        ": 15Hz max ping rate.\n\t" + std::to_string(pingRateHighest) + ": 40Hz max ping rate.\n\t" +
        std::to_string(pingRateLow) + ": 5Hz max ping rate.\n\t" + std::to_string(pingRateLowest) + ": 2Hz max ping rate.\n\t" +
        std::to_string(pingRateStandby) + ": Standby mode (no ping fire)"};
const IntParam NBEAMS = {"nbeams", 0, 1, 1,
    "Number of ping beams.\n"
    "\t0: Oculus outputs 256 beams.\n"
    "\t1: Oculus outputs 512 beams."};
const IntParam GAMMA_CORRECTION = {"gamma_correction", 1, 255, 153, "Gamma correction, min=1, max=255."};

const std::vector<IntParam> INT = {FREQUENCY_MODE, PING_RATE, NBEAMS, GAMMA_CORRECTION};

struct DoubleParam {
  const std::string name;
  const double min;
  const double max;
  const double step;
  const double default_val;
  const std::string desc;
};

const DoubleParam RANGE = {"range", .3, 30., .1, 5., "Sonar range (in meters), min=0.3, max=40.0."};
const DoubleParam GAIN_PERCENT = {"gain_percent", .1, 100., .1, 50., "Gain percentage (%), min=0.1, max=100.0."};
const DoubleParam SOUND_SPEED = {"sound_speed", 1400., 1600., .1, 1500.,
    "Sound speed (in m/s, set to 0 for it to be calculated using salinity), min=1400.0, max=1600.0."};
const DoubleParam SALINITY = {"salinity", 0., 100., .1, 0.,
    "Salinity (in parts per thousand (ppt,ppm,g/kg), "
    "used to calculate sound speed if needed), min=0.0, max=100"};

const std::vector<DoubleParam> DOUBLE = {RANGE, GAIN_PERCENT, SOUND_SPEED, SALINITY};

}  // namespace params

class OculusApp {
public:
  OculusApp();
  ~OculusApp();

protected:
  const std::vector<std::string> dynamic_parameters_names_{
      params::FREQUENCY_MODE.name, params::PING_RATE.name,
      params::NBEAMS.name, params::GAIN_ASSIT.name, params::RANGE.name, 
      params::GAMMA_CORRECTION.name, params::GAIN_PERCENT.name,
      params::SOUND_SPEED.name, params::USE_SALINITY.name, params::SALINITY.name,
      params::SWITCH_SCIENTIFIC_DATA_MODE.name,
      params::SET_GAIN_BOOST.name,
      params::SET_TRANSMIT_BOOST.name,
      "run"};

  // Single source of truth for parameters - represents actual sonar state
  SonarParameters currentSonarParameters_;
  oculus::SonarDriver::PingConfig currentConfig_;

  bool is_running_;  // State value. Same value as ros parameter "run"
  bool is_overheating_ = false;  // State value

  mutable std::shared_mutex param_mutex_;  // Protects parameter access

private:

  // Attributes 
  std::shared_ptr<Logger> console_log_; // logger instance to console

  std::shared_ptr<oculus::SonarDriver> sonar_driver_; // sonar driver instance for interface
  oculus::AsyncService io_service_; // cabled connnection interface utils 

  std::shared_ptr<OculusViewer> viewer_;
  
  // Methods
  // Callback handlers
  void handleStatus(const OculusStatusMsg& status);
  void handlePing(const oculus::PingMessage::ConstPtr& ping);
  void handleDummy();

  // Mode control
  void enableRunMode();
  void disableRunMode();
  void checkOverheating(const double& new_temperature);
  
  // Flag manipulation
  void setMinimalFlags(uint8_t& flags) const;
  void checkMinimalFlags(const uint8_t& flags) const;
  
  // Parameter handling
  void updateSonarParametersFromPingConfig(const oculus::SonarDriver::PingConfig& feedback);

};

#endif  // OCULUS_APP_HPP
