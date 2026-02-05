#include "oculus_app/OculusApp.hpp"


OculusApp::OculusApp()
  : is_running_(params::RUN_MODE_DEFAULT_VALUE)
{

  // Initialize Driver Instance 
  this->console_log_ = std::make_shared<Logger>(); 
  console_log_->log(INFO,"Initializing OculusApp...");

  this->sonar_driver_ = std::make_shared<oculus::SonarDriver>(this->io_service_.io_service());
  console_log_->log(INFO,"Connecting with the Oculus Sonar...");

  this->io_service_.start();

  if (!this->sonar_driver_->wait_next_message()) {  // Non-blocking function making connection with the sonar.
    console_log_->log(WARNING,"Timeout reached while waiting for a connection to the Oculus sonar.\nIs it properly connected ?");
  }

  console_log_->log(INFO,"Attempting 5 new connections with 5 seconds timeout...");
  int connection_tentative = 1; 

  while (connection_tentative <= 5 && !this->sonar_driver_->connected())  // Blocking while waiting the connected with the sonar.
  {
    console_log_->log(INFO, "Connecting with the Oculus Sonar... Tentative = " + std::to_string(connection_tentative));
    const int sleepWhileConnecting = 5000;
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepWhileConnecting));

    connection_tentative ++;
  }

  if (!this->sonar_driver_->connected()) {
    console_log_->log(ERROR,"Connection Failed");
    return;  // Exit constructor early
  }
  console_log_->log(INFO,"Connection Established");

  // Initializing the Viewer instance 
  this->viewer_ = std::make_shared<OculusViewer>(this->console_log_);

  // Configure Sonar 
  // Initialize default config
  this->currentConfig_.head.oculusId = 0x4f53;
  this->currentConfig_.head.srcDeviceId = 0;
  this->currentConfig_.head.dstDeviceId = 1042;
  this->currentConfig_.head.msgId = messageSimpleFire;
  this->currentConfig_.head.msgVersion = 2;
  this->currentConfig_.head.payloadSize = sizeof(PingConfig) - sizeof(OculusMessageHeader);
  this->currentConfig_.head.partNumber = 0;
  this->currentConfig_.masterMode = 2;
  this->currentConfig_.pingRate = pingRateNormal;
  this->currentConfig_.networkSpeed = 0xff;
  this->currentConfig_.gammaCorrection = 0;
  this->currentConfig_.flags = (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3);
  this->currentConfig_.range = 5.0;
  this->currentConfig_.gainPercent = 50.0;
  this->currentConfig_.speedOfSound = 0;
  this->currentConfig_.salinity = 0;


  // Get initial configuration from sonar
  updateSonarParametersFromPingConfig(this->sonar_driver_->current_ping_config());

  // Set up driver callbacks
  console_log_->log(INFO, "Setting up sonar callbacks");
  this->sonar_driver_->add_status_callback(
      std::bind(&OculusApp::handleStatus, this, std::placeholders::_1));
  this->sonar_driver_->add_ping_callback(
      std::bind(&OculusApp::handlePing, this, std::placeholders::_1));
  this->sonar_driver_->add_dummy_callback(
      std::bind(&OculusApp::handleDummy, this));

  console_log_->log(INFO, "OculusApp Initialized Successfully");

  if (is_running_) {
    this->enableRunMode();
  }
}


OculusApp::~OculusApp()
{
  // Stop Connection 
  console_log_->log(INFO, "Shutting down OculusApp...");
  this->io_service_.stop();
  console_log_->log(INFO, "OculusApp stopped");
}

void OculusApp::enableRunMode()
{
  std::unique_lock lock(param_mutex_);
  this->sonar_driver_->resume();
  is_running_ = true;
  console_log_->log(INFO, "Entering run mode");
}

void OculusApp::disableRunMode()
{
  std::unique_lock lock(param_mutex_);
  this->sonar_driver_->standby();
  is_running_ = false;
  console_log_->log(INFO, "Going to standby mode");
}

void OculusApp::setMinimalFlags(uint8_t& flags) const
{
  flags |= flagByte::RANGE_AS_METERS   // always in meters
         | flagByte::SIXTEEN_BIT_IMG   // 16 bits
         | flagByte::SEND_GAINS        // force send gain
         | flagByte::SIMPLE_PING;      // use simple ping

  // Gain assist handling based on frequency mode
  if (currentSonarParameters_.gain_assist && 
      currentSonarParameters_.frequency_mode == params::FREQUENCY_MODE.max) {
    flags |= flagByte::GAIN_ASSIST;
  } else {
    flags &= ~flagByte::GAIN_ASSIST;
  }

  // Bit 7 must be false to avoid broken connection
  flags &= ~0x80;
}

void OculusApp::checkMinimalFlags(const uint8_t& flags) const
{
  if (!(flags & flagByte::RANGE_AS_METERS)) {
    console_log_->log(ERROR, "Range is interpreted as percent while driver assumes meters.");
  }
  if (!(flags & flagByte::SEND_GAINS)) {
    console_log_->log(ERROR, "Sonar doesn't send gain - data incomplete.");
  }
  if (!(flags & flagByte::SIMPLE_PING)) {
    console_log_->log(ERROR, "Sonar doesn't use simple ping message.");
  }
}

void OculusApp::checkOverheating(const double& new_temperature)
{
  is_overheating_ = new_temperature >= params::TEMPERATURE_STOP_DEFAULT_VALUE;
}

void OculusApp::handleStatus(const OculusStatusMsg& status)
{
  if (status.partNumber != OculusPartNumberType::partNumberM3000d) {
    console_log_->log(ERROR, "Sonar version seems different than M3000d. Driver may not work correctly.");
  }

  if (!is_running_) {
    checkOverheating(status.temperature6);
    console_log_->log(DEBUG, "Temperature: " + std::to_string(status.temperature6) + "°C, Pressure: " + std::to_string(status.pressure) + " Pa");
  }
}

void OculusApp::handlePing(const oculus::PingMessage::ConstPtr& ping)
{
  // Check if the sonar must go in standby mode
  checkOverheating(ping->temperature());
  
  if (!is_running_) {
    disableRunMode();
    console_log_->log(DEBUG, "Not running - ping_rate mode is set to standby");
  } else if (currentSonarParameters_.ping_rate == pingRateStandby) {
    console_log_->log(INFO, "ping_rate mode is set to standby");
    disableRunMode();
  } else if (is_overheating_) {
    console_log_->log(CRITICAL, 
        "Temperature too high (" + std::to_string(ping->temperature()) + "°C). " +
        "Make sure sonar is underwater. Limit: " + std::to_string(params::TEMPERATURE_STOP_DEFAULT_VALUE) + "°C");
    disableRunMode();
  } else if (ping->temperature() >= params::TEMPERATURE_WARN_DEFAULT_VALUE) {
    console_log_->log(WARNING, 
        "Temperature high (" + std::to_string(ping->temperature()) + "°C). " +
        "Make sure sonar is underwater. Limit: " + std::to_string(params::TEMPERATURE_STOP_DEFAULT_VALUE) + "°C");
  }

  // Update current parameters with ping information
  std::unique_lock lock(param_mutex_);
  currentSonarParameters_.frequency_mode = ping->master_mode();
  currentSonarParameters_.range = ping->range();
  currentSonarParameters_.gain_percent = ping->gain_percent();
  currentSonarParameters_.sound_speed = ping->speed_of_sound_used();
  lock.unlock();

  // Log ping info
  // console_log_->log(INFO, 
  //     "Ping #" + std::to_string(ping->ping_index()) + 
  //     " - Range: " + std::to_string(ping->range()) + "m" +
  //     " - Temp: " + std::to_string(ping->temperature()) + "°C" +
  //     " - Beams: " + std::to_string(ping->bearing_count()));

  // Display ping in viewer
  if (viewer_) {
      viewer_->displayPing(ping);
  }
}

void OculusApp::handleDummy()
{
  if (is_running_ && !is_overheating_ && 
      currentSonarParameters_.ping_rate != pingRateStandby) {
    console_log_->log(INFO, "Exiting standby mode");
    enableRunMode();
  }
}

void OculusApp::updateSonarParametersFromPingConfig(const oculus::SonarDriver::PingConfig& feedback)
{
  std::unique_lock lock(param_mutex_);
  
  checkMinimalFlags(feedback.flags);

  currentSonarParameters_.frequency_mode = feedback.masterMode;
  currentSonarParameters_.ping_rate = feedback.pingRate;
  currentSonarParameters_.nbeams = (feedback.flags & flagByte::NBEAMS) ? 1 : 0;
  currentSonarParameters_.gain_assist = (feedback.flags & flagByte::GAIN_ASSIST) != 0;
  
  currentSonarParameters_.range = feedback.range;
  currentSonarParameters_.gamma_correction = feedback.gammaCorrection;
  currentSonarParameters_.gain_percent = feedback.gainPercent;
  currentSonarParameters_.sound_speed = feedback.speedOfSound;
  currentSonarParameters_.salinity = feedback.salinity;
  currentSonarParameters_.use_salinity = (feedback.speedOfSound == 0.0);
}
