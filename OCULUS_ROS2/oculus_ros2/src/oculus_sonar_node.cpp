#define BOOST_BIND_GLOBAL_PLACEHOLDERS

#include <oculus_ros2/oculus_sonar_node.hpp>

using SonarDriver = oculus::SonarDriver;

thread_local bool OculusSonarNode::syncing_from_driver_ = false;

namespace {

// Period of the sonar -> ROS parameter synchronisation (runs on the executor thread).
constexpr std::chrono::milliseconds SYNC_PERIOD{100};

// Pings received within this window after a config answer may still carry the OLD settings
// (they were already in flight). They are not used to update the sonar state.
constexpr std::int64_t SETTLE_TIME_NS = 500LL * 1000 * 1000;

std::int64_t steadyNowNs() {
  return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

// Descriptor for parameters that are only read at startup (changing them at runtime is rejected by ROS).
rcl_interfaces::msg::ParameterDescriptor readOnlyDescriptor(const std::string& description) {
  rcl_interfaces::msg::ParameterDescriptor descriptor;
  descriptor.description = description;
  descriptor.read_only = true;
  return descriptor;
}

}  // namespace

OculusSonarNode::OculusSonarNode()
  : Node("oculus_sonar"),
    is_running_(this->declare_parameter<bool>("run", params::RUN_MODE_DEFAULT_VALUE)),
    frame_id_(this->declare_parameter<std::string>("frame_id", "sonar", readOnlyDescriptor("Frame id of the published messages."))),
    temperature_warn_limit_(this->declare_parameter<double>(
        "temperature_warn", params::TEMPERATURE_WARN_DEFAULT_VALUE, readOnlyDescriptor("Temperature (C) above which a warning is logged."))),
    temperature_stop_limit_(this->declare_parameter<double>(
        "temperature_stop", params::TEMPERATURE_STOP_DEFAULT_VALUE, readOnlyDescriptor("Temperature (C) above which the sonar goes in standby."))) {
  
  RCLCPP_INFO(this->get_logger(), "Initializing the node ...");

  // Publishers
  this->ping_publisher_ = this->create_publisher<oculus_interfaces::msg::Ping>("ping", 10);
  this->oculus_ping_publisher_ = this->create_publisher<oculus_interfaces::msg::OculusPing2>("oculus_ping", 10);
  this->status_publisher_ = this->create_publisher<oculus_interfaces::msg::OculusStatus>("status", 10);
  this->temperature_publisher_ = this->create_publisher<sensor_msgs::msg::Temperature>("temperature", 1);
  this->pressure_publisher_ = this->create_publisher<sensor_msgs::msg::FluidPressure>("pressure", 1);

  // Connection to the sonar
  this->sonar_driver_ = std::make_shared<SonarDriver>(this->io_service_.io_service());
  
  RCLCPP_INFO(this->get_logger(), " Connecting with the Oculus Sonar...");
  this->io_service_.start();
  if (!this->sonar_driver_->wait_next_message()) {  // Non-blocking function making connection with the sonar.
    std::cerr << "Timeout reached while waiting for a connection to the Oculus sonar. "
              << "Is it properly connected ?" << std::endl;
  }

  // Block until connected (or until Ctrl-C).
  while (rclcpp::ok() && !this->sonar_driver_->connected())  // Blocking while waiting the connected with the sonar.
  {
    // std::cerr << "Timeout reached while waiting for a connection to the Oculus sonar. "
    //           << "Is it properly connected ?" << std::endl;
    const int sleepWhileConnecting = 1000;
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepWhileConnecting));
  }
  if (!this->sonar_driver_->connected()) {
    throw std::runtime_error("Connection to the Oculus sonar failed.");
    return;  // Exit constructor early
  }

  RCLCPP_INFO(this->get_logger(), " Connection Established !");

  // send config to Oculus sonar and wait for feedback
  // Default fire message (baseline for every later request)
  this->currentConfig_.head.oculusId = 0x4f53;
  this->currentConfig_.head.srcDeviceId = 0;            // The device id of the source
  this->currentConfig_.head.dstDeviceId = 1042;         // The device id of the destination
  this->currentConfig_.head.msgId = messageSimpleFire;  // Message identifier
  this->currentConfig_.head.msgVersion = 2;
  this->currentConfig_.head.payloadSize = sizeof(PingConfig) - sizeof(OculusMessageHeader);
  this->currentConfig_.head.partNumber = 0;
  this->currentConfig_.masterMode = 2;
  this->currentConfig_.pingRate = pingRateNormal;           
  this->currentConfig_.networkSpeed = 0xff;       // Max network speed in Mbs, 0x00 or 0xff = use link speed
  this->currentConfig_.gammaCorrection = 0;    // The gamma correction - 255 is equal to a gamma correction of 1.0 
  this->currentConfig_.flags = (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3);
  this->currentConfig_.range = 5.0;             // Range demand (m, because flag bit 0 is set)
  this->currentConfig_.gainPercent = 50.0;      // The percentage gain 
  this->currentConfig_.speedOfSound = 0;       // 0 = sonar computes it from salinity
  this->currentConfig_.salinity = 0;           // ppt, used by the internal sound speed calculation
  // this->currentConfig_.extFlags = 0;
  // this->currentConfig_.reserved0[0] = 0;
  // this->currentConfig_.reserved0[1] = 0;
  // this->currentConfig_.reserved1[0] = 0;
  // this->currentConfig_.reserved1[1] = 0;
  // this->currentConfig_.reserved1[2] = 0;
  // this->currentConfig_.reserved1[3] = 0;
  // this->currentConfig_.reserved1[4] = 0;
 
  // Declare the dynamic parameters (YAML / launch overrides are applied here) 
  // No parameter callback is registered yet, so nothing is sent to the sonar at this stage.
  // Out-of-range overrides make declare_parameter throw (fail fast on a bad YAML).
  RCLCPP_INFO(this->get_logger(), " Setting up parameters ...");

  for (const params::BoolParam& param : params::BOOL) {
    if (!this->has_parameter(param.name)) {
      rcl_interfaces::msg::ParameterDescriptor param_desc;
      param_desc.name = param.name;
      param_desc.type = rclcpp::ParameterType::PARAMETER_BOOL;
      param_desc.description = param.desc;
      this->declare_parameter<bool>(param.name, param.default_val, param_desc);
    }
  }
  for (const params::IntParam& param : params::INT) {
    if (!this->has_parameter(param.name)) {
      rcl_interfaces::msg::ParameterDescriptor param_desc;
      param_desc.name = param.name;
      param_desc.type = rclcpp::ParameterType::PARAMETER_INTEGER;
      param_desc.description = param.desc;
      rcl_interfaces::msg::IntegerRange range;
      range.set__from_value(param.min).set__to_value(param.max).set__step(1);
      param_desc.integer_range = {range};
      this->declare_parameter<int>(param.name, param.default_val, param_desc);
    }
  }
  for (const params::DoubleParam& param : params::DOUBLE) {
    if (!this->has_parameter(param.name)) {
      rcl_interfaces::msg::ParameterDescriptor param_desc;
      param_desc.name = param.name;
      param_desc.type = rclcpp::ParameterType::PARAMETER_DOUBLE;
      param_desc.description = param.desc;
      // FloatingPointRange (not IntegerRange) is what rclcpp enforces on doubles.
      // step = 0 means "continuous"; a non-zero step would force values onto a grid.
      rcl_interfaces::msg::FloatingPointRange range;
      //range.set__from_value(param.min).set__to_value(param.max).set__step(param.step);
      range.set__from_value(param.min).set__to_value(param.max).set__step(0.0);
      param_desc.floating_point_range = {range};
      this->declare_parameter<double>(param.name, param.default_val, param_desc);
    }
  }

  // Get the current sonar config
  // Initial state 
  // 1. What the sonar currently uses.
  {
    std::lock_guard<std::mutex> lock(state_mutex_);
    updateLocalParameters(currentSonarParameters_, this->sonar_driver_->current_ping_config());
  }
  // 2. What ROS wants (YAML / defaults). Filled BEFORE the replay so that order-dependent
  // checks (e.g. sound_speed reads use_salinity) see the requested values, not garbage.
  updateLocalParameters(currentRosParameters_, this->get_parameters(dynamic_parameters_names_));
  
  // 3. Replay every parameter once through the normal validation path to push the
  //     requested configuration to the sonar. Failures are logged, not fatal: the sync timer
  //     will later copy the sonar's real value back into the ROS parameter.
  for (const std::string& param_name : dynamic_parameters_names_) {
    const auto result = setConfigCallback(this->get_parameters(std::vector<std::string>{param_name}));
    if (!result.successful) {
      RCLCPP_WARN_STREAM(this->get_logger(),
          "Initial value of '" << param_name << "' was not applied to the sonar: " << result.reason);
    }
  }
  

  // Callbacks 
  // From now on, parameters set by users go through setConfigCallback.
  this->param_cb_ = this->add_on_set_parameters_callback(std::bind(&OculusSonarNode::setConfigCallback, this,
      std::placeholders::_1));  // TODO(hugoyvrn, to move before parameters initialisation ?)

  RCLCPP_INFO(this->get_logger(), " Setting up driver callbacks ...");
  this->sonar_driver_->add_status_callback(std::bind(&OculusSonarNode::publishStatus, this, std::placeholders::_1));
  this->sonar_driver_->add_ping_callback(std::bind(&OculusSonarNode::publishPing, this, std::placeholders::_1));
  // callback on dummy messages to reactivate the pings as needed
  this->sonar_driver_->add_dummy_callback(std::bind(&OculusSonarNode::handleDummy, this));
  
  // Sonar -> ROS parameter sync, on the executor thread (never on the io thread).
  this->sync_timer_ = this->create_wall_timer(SYNC_PERIOD, std::bind(&OculusSonarNode::syncRosParameters, this));

  // Initial run state 
  // Honour "run" from the YAML (the original always resumed the sonar here).
  if (is_running_ && sonarParametersSnapshot().ping_rate != pingRateStandby) {
    this->enableRunMode();
  } else {
    this->disableRunMode();
  }

  RCLCPP_INFO(this->get_logger(), " Node initialized");
}

OculusSonarNode::~OculusSonarNode() {
  this->io_service_.stop();
}

// Wake the sonar up. `is_running_` is already true when this is called (user intent).
void OculusSonarNode::enableRunMode() {
  this->sonar_driver_->resume();
}

/// Put the sonar in standby and record that we are no longer running.
/// The ROS parameter "run" is updated later by syncRosParameters (executor thread).
void OculusSonarNode::disableRunMode() {
  this->sonar_driver_->standby();
  is_running_ = false;
  RCLCPP_INFO(this->get_logger(), "Going to standby mode");
}

void OculusSonarNode::checkOverheating(const double& new_temperature) {
  is_overheating_ = new_temperature >= temperature_stop_limit_;
}

// Flags

/// Force the flag bits this driver relies on. `master_mode` is the mode of the config being sent
/// (not the previous one), so a frequency change is handled correctly.
void OculusSonarNode::setMinimalFlags(uint8_t& flags, int master_mode) const {
  flags |= flagByte::RANGE_AS_METERS   // range is always in meters
         | flagByte::SIXTEEN_BIT_IMG   // 16 bit images
         | flagByte::SEND_GAINS        // the sonar must send the gains
         | flagByte::SIMPLE_PING;      // use the simple ping message

  if (master_mode == params::FREQUENCY_MODE.max) {
    // TODO(hugoyvrn): gain_assist does not work at high frequency, forced off.
    flags &= ~flagByte::GAIN_ASSIST;
  }

  // Bit 7 must be false: otherwise the connection breaks (Header reception error), answers become
  // very slow and the sonar has to be restarted.
  // flags | 0x02 make wird change (depending of the configuration)
  // flags |= 0x02;
  // flags &= ~0x02;
  // flags | 0x20 must be ???
  // flags |= 0x20;
  // flags &= ~0x20;
  // flags | 0x80 must be false to avoid broken connection (Header reception error) and very long param answers. Restart of the
  // sonar needed (even for IHM). tested with flags = 4d and flags = cd and flags = fd and flags = 19
  // flags |= 0x80;
  flags &= ~0x80;
}


void OculusSonarNode::checkMinimalFlags(const uint8_t& flags) const {
  if (!(flags & flagByte::RANGE_AS_METERS)) {
    RCLCPP_ERROR(get_logger(), "Range is attepreted as percent while ros driver assume range is interpreted as meters.");
  }
  if (!(flags & flagByte::SEND_GAINS)) {
    RCLCPP_ERROR(get_logger(), "The sonar don't send gain while ros driver assume gains are sended. Data is incomplete.");
  }
  if (!(flags & flagByte::SIMPLE_PING)) {
    RCLCPP_ERROR(get_logger(), "The sonar don't use simple ping message while ros driver assume simple ping are used.");
  }
}

void OculusSonarNode::publishStatus(const OculusStatusMsg& status) {
  if (status.partNumber != OculusPartNumberType::partNumberM3000d) {
    RCLCPP_ERROR_STREAM(get_logger(),
        "The sonar version seems to be different than M3000d."
        " This driver is not suppose to work with your sonar.");
  }

  static oculus_interfaces::msg::OculusStatus msg;
  oculus::toMsg(msg, status);
  this->status_publisher_->publish(msg);

  if (!is_running_) {
    checkOverheating(status.temperature6);

    sensor_msgs::msg::Temperature temperature_ros_msg;
    temperature_ros_msg.header.frame_id = frame_id_;
    temperature_ros_msg.header.stamp = this->now();
    temperature_ros_msg.temperature = status.temperature6;  // Measurement of the Temperature in Degrees Celsius
    temperature_ros_msg.variance = 0;  // 0 is interpreted as variance unknown
    this->temperature_publisher_->publish(temperature_ros_msg);

    sensor_msgs::msg::FluidPressure pressure_ros_msg;
    pressure_ros_msg.header.frame_id = frame_id_;
    pressure_ros_msg.header.stamp = this->now();
    pressure_ros_msg.fluid_pressure = status.pressure;  // Pressure reading in Pascals.
    pressure_ros_msg.variance = 0;  // 0 is interpreted as variance unknown
    this->pressure_publisher_->publish(pressure_ros_msg);
  }
}

int OculusSonarNode::get_subscription_count() const {
  return this->ping_publisher_->get_subscription_count();
}

void OculusSonarNode::publishPing(const oculus::PingMessage::ConstPtr& ping) {
 // 1. Decide whether the sonar must go in standby.
  checkOverheating(ping->temperature());
  const int ping_rate = sonarParametersSnapshot().ping_rate;

  if (!is_running_) {
    disableRunMode();
  } else if (ping_rate == pingRateStandby) {
    RCLCPP_INFO_STREAM(this->get_logger(), "ping_rate is set to " << pingRateStandby << " (standby).");
    disableRunMode();
  } else if (is_overheating_) {
    RCLCPP_FATAL_STREAM(this->get_logger(), "Temperature of sonar is too high ("
                                                << ping->temperature()
                                                << " C). Make sure the sonar is underwater. Security limit set at "
                                                << temperature_stop_limit_ << " C");
    disableRunMode();
  } else if (ping->temperature() >= temperature_warn_limit_) {
    RCLCPP_WARN_STREAM(this->get_logger(), "Temperature of sonar is high ("
                                               << ping->temperature()
                                               << " C). Make sure the sonar is underwater. Security limit set at "
                                               << temperature_stop_limit_ << " C");
  }

  // 2. Refresh the sonar state from the ping. Skipped right after a config change: pings already
  //    in flight still carry the old values and would make the ROS parameter flip back.
  if (steadyNowNs() - last_config_answer_ns_.load() > SETTLE_TIME_NS) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    currentSonarParameters_.frequency_mode = ping->master_mode();
    currentSonarParameters_.range = ping->range();
    currentSonarParameters_.gain_percent = ping->gain_percent();
    currentSonarParameters_.sound_speed = ping->speed_of_sound_used();
  }
  // The ROS parameters are updated by syncRosParameters (executor thread).

  // 3. Publish.
  static oculus_interfaces::msg::Ping msg;
  msg.header.frame_id = frame_id_;
  oculus::toMsg(msg, ping);
  this->ping_publisher_->publish(msg);

  static oculus_interfaces::msg::OculusPing2 msg_oculus;
  msg_oculus.header.frame_id = frame_id_;
  oculus::toMsg(msg_oculus, ping);
  this->oculus_ping_publisher_->publish(msg_oculus);

  sensor_msgs::msg::Temperature temperature_ros_msg;
  temperature_ros_msg.header = msg.header;
  temperature_ros_msg.temperature = msg.temperature;  // Measurement of the Temperature in Degrees Celsius
  temperature_ros_msg.variance = 0;  // 0 is interpreted as variance unknown
  this->temperature_publisher_->publish(temperature_ros_msg);

  sensor_msgs::msg::FluidPressure pressure_ros_msg;
  pressure_ros_msg.header = msg.header;
  pressure_ros_msg.fluid_pressure = msg.pressure;  // Absolute pressure reading in Pascals.
  pressure_ros_msg.variance = 0;  // 0 is interpreted as variance unknown
  this->pressure_publisher_->publish(pressure_ros_msg);
}

/// Called for dummy messages (what the sonar sends while in standby): wake it up when the user
/// wants it running, somebody listens and nothing prevents it.
void OculusSonarNode::handleDummy() {
  if (is_running_ && get_subscription_count() > 0 && !is_overheating_ &&
      sonarParametersSnapshot().ping_rate != pingRateStandby) {
    RCLCPP_INFO(this->get_logger(), "Exiting standby mode");
    enableRunMode();
  }
}

SonarParameters OculusSonarNode::sonarParametersSnapshot() const {
  std::lock_guard<std::mutex> lock(state_mutex_);
  return currentSonarParameters_;
}

/// Write a ROS parameter on behalf of the node itself, without going through the sonar
/// validation (setConfigCallback returns immediately while the flag is set). The flag is
/// thread_local: user requests handled on other threads are not affected.
bool OculusSonarNode::setParameterBypassingValidation(const rclcpp::Parameter& param) {
  rcl_interfaces::msg::SetParametersResult result;
  syncing_from_driver_ = true;
  try {
    result = this->set_parameter(param);
  } catch (const std::exception& e) {
    result.successful = false;
    result.reason = e.what();
  }
  syncing_from_driver_ = false;

  if (!result.successful) {
    RCLCPP_WARN_STREAM(this->get_logger(), "Could not mirror the sonar value into parameter '" << param.get_name()
                                                                                              << "': " << result.reason);
  }
  return result.successful;
}

/// Timer callback: make the ROS parameters follow the real sonar state.
void OculusSonarNode::syncRosParameters() {
  const SonarParameters sonar = sonarParametersSnapshot();

  updateRosConfigForParam<int>(currentRosParameters_.frequency_mode, sonar.frequency_mode, params::FREQUENCY_MODE.name);
  updateRosConfigForParam<double>(currentRosParameters_.range, sonar.range, params::RANGE.name);
  updateRosConfigForParam<double>(currentRosParameters_.gain_percent, sonar.gain_percent, params::GAIN_PERCENT.name);
  // With use_salinity the sonar computes (and changes with temperature) the sound speed: the ROS
  // sound_speed is then just the value to use once use_salinity is switched off.
  if (!sonar.use_salinity) {
    updateRosConfigForParam<double>(currentRosParameters_.sound_speed, sonar.sound_speed, params::SOUND_SPEED.name);
  }
  updateRosConfigForParam<int>(currentRosParameters_.ping_rate, sonar.ping_rate, params::PING_RATE.name);
  updateRosConfigForParam<bool>(currentRosParameters_.gain_assist, sonar.gain_assist, params::GAIN_ASSIT.name);
  updateRosConfigForParam<int>(currentRosParameters_.gamma_correction, sonar.gamma_correction, params::GAMMA_CORRECTION.name);
  updateRosConfigForParam<bool>(currentRosParameters_.use_salinity, sonar.use_salinity, params::USE_SALINITY.name);
  updateRosConfigForParam<double>(currentRosParameters_.salinity, sonar.salinity, params::SALINITY.name);

  // "run": the io thread may have switched is_running_ off (overheating, standby ping rate).
  const bool running = is_running_.load();
  if (this->get_parameter("run").as_bool() != running) {
    setParameterBypassingValidation(rclcpp::Parameter("run", running));
  }
}

/// Copy values from a list of ROS parameters into a SonarParameters struct.
void OculusSonarNode::updateLocalParameters(SonarParameters& parameters, const std::vector<rclcpp::Parameter>& new_parameters) {
  for (const rclcpp::Parameter& new_param : new_parameters) {
    const std::string& name = new_param.get_name();
    if (name == params::FREQUENCY_MODE.name) {
      parameters.frequency_mode = static_cast<int>(new_param.as_int());
    } else if (name == params::PING_RATE.name) {
      parameters.ping_rate = static_cast<int>(new_param.as_int());
    } else if (name == params::NBEAMS.name) {
      parameters.nbeams = static_cast<int>(new_param.as_int());
    } else if (name == params::GAIN_ASSIT.name) {
      parameters.gain_assist = new_param.as_bool();
    } else if (name == params::RANGE.name) {
      parameters.range = new_param.as_double();
    } else if (name == params::GAMMA_CORRECTION.name) {
      parameters.gamma_correction = static_cast<int>(new_param.as_int());
    } else if (name == params::GAIN_PERCENT.name) {
      parameters.gain_percent = new_param.as_double();
    } else if (name == params::SOUND_SPEED.name) {
      parameters.sound_speed = new_param.as_double();
    } else if (name == params::USE_SALINITY.name) {
      parameters.use_salinity = new_param.as_bool();
    } else if (name == params::SALINITY.name) {
      parameters.salinity = new_param.as_double();
    } else if (name == params::SWITCH_SCIENTIFIC_DATA_MODE.name) {
      parameters.scientific_mode = new_param.as_bool();
    } else if (name == params::SET_GAIN_BOOST.name) {
      parameters.gain_boost = new_param.as_bool();
    } else if (name == params::SET_TRANSMIT_BOOST.name) {
      parameters.transmit_boost = new_param.as_bool();
    } else if (name != "run") {
      RCLCPP_WARN_STREAM(get_logger(), "Wrong parameter to set: new_param = " << new_param << ". Not set");
    }
  }
}

void OculusSonarNode::updateLocalParameters(SonarParameters& parameters, SonarDriver::PingConfig feedback) {
  std::vector<rclcpp::Parameter> new_parameters;
  // OculusMessageHeader head;      // The standard message header
  // uint16_t oculusId;          // Fixed ID 0x4f53
  // uint16_t srcDeviceId;       // The device id of the source
  // uint16_t dstDeviceId;       // The device id of the destination
  // uint16_t msgId;             // Message identifier
  // uint16_t msgVersion;
  // uint32_t payloadSize;       // The size of the message payload (header not included)
  // uint16_t spare2;
  // uint8_t masterMode;            // mode 0 is flexi mode, needs full fire message (not available for third party developers)
  //                                // mode 1 - Low Frequency Mode (wide aperture, navigation)
  //                                // mode 2 - High Frequency Mode (narrow aperture, target identification)
  // uint8_t pingRate;              // Sets the maximum ping rate. was PingRateType
  // uint8_t networkSpeed;          // Used to reduce the network comms speed (useful for high latency shared links)
  // uint8_t gammaCorrection;       // 0 and 0xff = gamma correction = 1.0
  //                                // Set to 127 for gamma correction = 0.5
  // uint8_t flags;                 // bit 0: 0 = interpret range as percent, 1 = interpret range as meters
  //                                // bit 1: 0 = 8 bit data, 1 = 16 bit data  // inverted ?
  //                                // bit 2: 0 = won't send gain, 1 = send gain
  //                                // bit 3: 0 = send full return message, 1 = send simple return message
  //                                // bit 4: gain assist ? TODO
  //                                // bit 5: ? TODO
  //                                // bit 6: enable 512 beams
  //                                // bit 7: ? TODO
  // double range;                  // The range demand in percent or m depending on flags
  // double gainPercent;            // The gain demand
  // double speedOfSound;           // ms-1, if set to zero then internal calc will apply using salinity
  // double salinity;               // ppt, set to zero if we are in fresh water

  checkMinimalFlags(feedback.flags);

  new_parameters.push_back(rclcpp::Parameter(params::FREQUENCY_MODE.name, feedback.masterMode));
  new_parameters.push_back(rclcpp::Parameter(params::PING_RATE.name, feedback.pingRate));
  // The mask gives 0 or 0x40: convert to the 0/1 the parameter expects.
  new_parameters.push_back(rclcpp::Parameter(params::NBEAMS.name, (feedback.flags & flagByte::NBEAMS) ? 1 : 0));
  new_parameters.push_back(rclcpp::Parameter(params::GAIN_ASSIT.name, static_cast<bool>(feedback.flags & flagByte::GAIN_ASSIST)));
#ifdef _PINGV2_
  new_parameters.push_back(rclcpp::Parameter(params::RANGE.name, feedback.rangePercent));
#else
  new_parameters.push_back(rclcpp::Parameter(params::RANGE.name, feedback.range));
#endif
  new_parameters.push_back(rclcpp::Parameter(params::GAMMA_CORRECTION.name, static_cast<int>(feedback.gammaCorrection)));
  new_parameters.push_back(rclcpp::Parameter(params::GAIN_PERCENT.name, feedback.gainPercent));
  new_parameters.push_back(rclcpp::Parameter(params::SOUND_SPEED.name, feedback.speedOfSound));
  // The sonar has no "use_salinity" field: sound speed 0 is how that mode is requested.
  new_parameters.push_back(rclcpp::Parameter(params::USE_SALINITY.name, feedback.speedOfSound == 0.0));
  new_parameters.push_back(rclcpp::Parameter(params::SALINITY.name, feedback.salinity));
  
  updateLocalParameters(parameters, new_parameters);
}

/**
 * Apply ONE parameter change to the sonar.
 * Builds newConfig = last confirmed config + the change, sends it, then compares the sonar's
 * answer with the request. Anything not honoured marks `result` as failed (by reference!).
 */
void OculusSonarNode::sendParamToSonar(const rclcpp::Parameter& param, rcl_interfaces::msg::SetParametersResult& result) {
  SonarDriver::PingConfig newConfig = currentConfig_;
  const std::string& name = param.get_name();

  if (name == params::FREQUENCY_MODE.name) {
    RCLCPP_INFO_STREAM(this->get_logger(), "Updating frequency_mode to " << param.as_int() << " (1: 1.2MHz, 2: 3.0MHz).");
    newConfig.masterMode = param.as_int();
  } else if (name == params::PING_RATE.name) {
    RCLCPP_INFO_STREAM(this->get_logger(), "Updating ping_rate to " << param.as_int() << " (" + params::PING_RATE.desc + ").");
    newConfig.pingRate = param.as_int();
  } else if (name == params::NBEAMS.name) {
    RCLCPP_INFO_STREAM(this->get_logger(), "Updating nbeams to " << param.as_int() << " (0: 256 beams, 1: 512 beams).");
    if (param.as_int() == 0) {
      newConfig.flags &= ~flagByte::NBEAMS;
    } else {
      newConfig.flags |= flagByte::NBEAMS;
    }
  } else if (name == params::GAIN_ASSIT.name) {
    RCLCPP_INFO_STREAM(this->get_logger(), "Updating gain_assist to " << param.as_bool());
    if (param.as_bool()) {
      newConfig.flags |= flagByte::GAIN_ASSIST;
    } else {
      newConfig.flags &= ~flagByte::GAIN_ASSIST;
    }
  } else if (name == params::RANGE.name) {
    RCLCPP_INFO_STREAM(this->get_logger(), "Updating range to " << param.as_double() << " m.");
#ifdef _PINGV2_
    newConfig.rangePercent = param.as_double();
#else
    newConfig.range = param.as_double();
#endif
  } else if (name == params::GAMMA_CORRECTION.name) {
    RCLCPP_INFO_STREAM(this->get_logger(), "Updating gamma_correction to " << param.as_int());
    newConfig.gammaCorrection = param.as_int();
  } else if (name == params::GAIN_PERCENT.name) {
    RCLCPP_INFO_STREAM(this->get_logger(), "Updating gain_percent to " << param.as_double() << "%.");
    newConfig.gainPercent = param.as_double();
  } else if (name == params::USE_SALINITY.name) {
    RCLCPP_INFO_STREAM(this->get_logger(), "Updating use_salinity to " << param.as_bool());
    // true: speed 0 = sonar computes it. false: use the sound_speed currently set in ROS.
    newConfig.speedOfSound = param.as_bool() ? 0.0 : currentRosParameters_.sound_speed;
  } else if (name == params::SOUND_SPEED.name) {
    const double v = param.as_double();
    if (v < detail::FLOAT_TOLERANCE) {
      RCLCPP_INFO(this->get_logger(), "sound_speed is 0: the sonar computes it from salinity.");
      newConfig.speedOfSound = 0.0;
    } else if (currentRosParameters_.use_salinity) {
      RCLCPP_INFO_STREAM(this->get_logger(), "sound_speed set to " << v
                                                                   << " m/s but ignored while use_salinity is true.");
    } else {
      RCLCPP_INFO_STREAM(this->get_logger(), "Updating sound_speed to " << v << " m/s.");
      newConfig.speedOfSound = v;
    }
  } else if (name == params::SALINITY.name) {
    // Was missing in the original: the set was accepted but never sent.
    RCLCPP_INFO_STREAM(this->get_logger(), "Updating salinity to " << param.as_double() << " ppt.");
    newConfig.salinity = param.as_double();
  }

  setMinimalFlags(newConfig.flags, newConfig.masterMode);
  RCLCPP_DEBUG_STREAM(this->get_logger(), "Requested config:\n" << newConfig);

  // Blocking round trip to the sonar. No lock is held here: the io thread must stay free to
  // receive the answer. ASSUMPTION: request_ping_config returns the sonar's confirmed config
  // (check what the driver does on timeout: return the old config, or throw).
  SonarDriver::PingConfig feedback = currentConfig_;
  try {
    feedback = this->sonar_driver_->request_ping_config(newConfig);
  } catch (const std::exception& e) {
    result.successful = false;
    result.reason.append(std::string("Request to the sonar failed: ") + e.what() + "\n");
    RCLCPP_ERROR_STREAM(this->get_logger(), "Request to the sonar failed: " << e.what());
    return;
  }
  last_config_answer_ns_ = steadyNowNs();
  RCLCPP_DEBUG_STREAM(this->get_logger(), "Sonar feedback:\n" << feedback);

  currentConfig_ = feedback;
  {
    std::lock_guard<std::mutex> lock(state_mutex_);
    updateLocalParameters(currentSonarParameters_, feedback);
  }
  checkMinimalFlags(feedback.flags);

  // Did the sonar honour the request? (result is a reference: the verdict reaches ROS.)
  handleFeedbackForParam<int>(result, param, newConfig.masterMode, feedback.masterMode, params::FREQUENCY_MODE.name);
  // pingRate is not compared: the sonar answer is unreliable for it (TODO(???)).
  handleFeedbackForParam<bool>(result, param, (newConfig.flags & flagByte::GAIN_ASSIST) ? 1 : 0,
      (feedback.flags & flagByte::GAIN_ASSIST) ? 1 : 0, params::GAIN_ASSIT.name);
  handleFeedbackForParam<int>(result, param, (newConfig.flags & flagByte::NBEAMS) ? 1 : 0,
      (feedback.flags & flagByte::NBEAMS) ? 1 : 0, params::NBEAMS.name);
#ifdef _PINGV2_
  handleFeedbackForParam<double>(result, param, newConfig.rangePercent, feedback.rangePercent, params::RANGE.name);
#else
  handleFeedbackForParam<double>(result, param, newConfig.range, feedback.range, params::RANGE.name);
#endif
  handleFeedbackForParam<int>(result, param, newConfig.gammaCorrection, feedback.gammaCorrection, params::GAMMA_CORRECTION.name);
  handleFeedbackForParam<double>(result, param, newConfig.gainPercent, feedback.gainPercent, params::GAIN_PERCENT.name);
  handleFeedbackForParam<double>(result, param, newConfig.speedOfSound, feedback.speedOfSound, params::SOUND_SPEED.name);
  handleFeedbackForParam<double>(result, param, newConfig.salinity, feedback.salinity, params::SALINITY.name);

  if (feedback.pingRate == pingRateStandby && is_running_) {
    is_running_ = false;  // The sonar is put in standby by the next ping callback
  }
}

/**
 * Parameter validation callback (executor thread). Parameters must be set one at a time.
 * Returns successful = false to make ROS refuse the change and keep the old value.
 */
rcl_interfaces::msg::SetParametersResult OculusSonarNode::setConfigCallback(const std::vector<rclcpp::Parameter>& parameters) {
  rcl_interfaces::msg::SetParametersResult result;
  result.successful = true;
  result.reason = "";

  // The node itself is mirroring a sonar value into ROS: accept, nothing to send.
  // (Checked first, before any other logic.)
  if (syncing_from_driver_) {
    return result;
  }

  if (parameters.size() != 1) {
    RCLCPP_WARN(get_logger(), "You should set parameters one by one.");
    result.successful = false;
    result.reason = "Parameters should be set one by one";
    return result;
  }

  const rclcpp::Parameter& param = parameters.front();
  const std::string& name = param.get_name();

  if (name == "run") {
    // Only switching ON needs checks. (The original also checked when switching off while
    // already stopped, which made "run:=false" fail without subscribers.)
    if (param.as_bool() && !is_running_) {
      const int ping_rate = sonarParametersSnapshot().ping_rate;
      if (get_subscription_count() == 0 || is_overheating_ || ping_rate == pingRateStandby) {
        result.successful = false;
        result.reason = "The conditions to go in run mode are not met.";
        if (get_subscription_count() == 0) {
          result.reason += " There is no subscriber to the ping topic.";
        }
        if (is_overheating_) {
          result.reason += " Temperature of sonar is too high. Make sure the sonar is underwater. Security limit set at " +
                           std::to_string(temperature_stop_limit_) + " C.";
        }
        if (ping_rate == pingRateStandby) {
          result.reason += " ping_rate is set to " + std::to_string(pingRateStandby) + " (standby).";
        }
        return result;
      }
    }
    // The actual sonar wake-up / standby is done by handleDummy / publishPing.
    is_running_ = param.as_bool();
    return result;
  }

  if (std::find(dynamic_parameters_names_.begin(), dynamic_parameters_names_.end(), name) == dynamic_parameters_names_.end()) {
    return result;  // Not a sonar parameter (e.g. read-only ones): nothing to do.
  }

  // Parameters declared but not implemented yet: refuse "true", accept "false" (nothing to send).
  if (name == params::SWITCH_SCIENTIFIC_DATA_MODE.name || name == params::SET_GAIN_BOOST.name ||
      name == params::SET_TRANSMIT_BOOST.name) {
    if (param.as_bool()) {
      result.successful = false;
      result.reason = name + " is not implemented in this driver version.";
      return result;
    }
    updateLocalParameters(currentRosParameters_, parameters);
    return result;
  }

  // QUICK FIX TODO(hugoyvrn): gain_assist is unreliable, so forbid the combinations that break.
  // Now sets successful = false (the original only set the reason, so ROS kept a value the
  // sonar never applied).
  const SonarParameters sonar = sonarParametersSnapshot();
  if (name == params::FREQUENCY_MODE.name && sonar.gain_assist && param.as_int() != sonar.frequency_mode) {
    result.successful = false;
    result.reason = "You must set gain_assist to false before changing frequency (TODO: to fix).";
    return result;
  }
  if (name == params::GAIN_ASSIT.name && param.as_bool() && sonar.frequency_mode == params::FREQUENCY_MODE.max) {
    result.successful = false;
    result.reason = "gain_assist is not available in frequency_mode " + std::to_string(params::FREQUENCY_MODE.max) + ".";
    return result;
  }

  if (name == params::SOUND_SPEED.name) {
    const double v = param.as_double();
    if (v >= detail::FLOAT_TOLERANCE && v < 1400.0) {
      result.successful = false;
      result.reason = "sound_speed must be 0 (computed from salinity) or between 1400.0 and 1600.0.";
      return result;
    }
  }
  if (name == params::USE_SALINITY.name && !param.as_bool() &&
      currentRosParameters_.sound_speed < detail::FLOAT_TOLERANCE) {
    result.successful = false;
    result.reason = "Set sound_speed (1400.0 to 1600.0) before disabling use_salinity.";
    return result;
  }

  sendParamToSonar(param, result);

  if (result.successful) {
    // Accepted by the sonar: remember it as the value held by ROS.
    updateLocalParameters(currentRosParameters_, parameters);
  }
  return result;
}

int main(int argc, char* argv[]) {
  rclcpp::init(argc, argv);
  int return_code = 0;
  try {
    // Single-threaded executor on purpose: parameter callbacks and the sync timer never overlap.
    rclcpp::spin(std::make_shared<OculusSonarNode>());
  } catch (const std::exception& e) {
    RCLCPP_FATAL(rclcpp::get_logger("oculus_sonar"), "Node terminated: %s", e.what());
    return_code = 1;
  }
  rclcpp::shutdown();
  return return_code;
}