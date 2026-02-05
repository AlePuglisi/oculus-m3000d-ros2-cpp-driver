#include "oculus_app/OculusViewer.hpp"


OculusViewer::OculusViewer()
{
  console_log_->log(INFO,"Initializing OculusViewer...");

  // Initialize Logger Instance 
  this->console_log_ = std::make_shared<Logger>(); 
  

  console_log_->log(INFO,"...OculusViewer Initialized");

}


OculusViewer::~OculusViewer()
{
    // Stop Connection 
    console_log_->log(INFO,"OculusViewer stopped");
    this->io_service_.stop();
}


void OculusViewer::Something()
{
    console_log_->log(INFO, "Do something");
}