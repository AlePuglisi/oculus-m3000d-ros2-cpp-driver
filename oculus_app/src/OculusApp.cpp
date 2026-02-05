#include "oculus_app/OculusApp.hpp"


OculusApp::OculusApp()
{
  console_log_->log(INFO,"Initializing OculusApp...");
  // Initialize Driver Instance 
  this->sonar_driver_ = std::make_shared<oculus::SonarDriver>(this->io_service_.io_service());
  this->console_log_ = std::make_shared<Logger>(); 
  
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

  // Configure Sonar 

  console_log_->log(INFO,"...OculusApp Initialized");
}


OculusApp::~OculusApp()
{
    // Stop Connection 
    console_log_->log(INFO,"OculusApp stopped");
    this->io_service_.stop();
}


void OculusApp::Something()
{
    console_log_->log(INFO, "Do something");
}