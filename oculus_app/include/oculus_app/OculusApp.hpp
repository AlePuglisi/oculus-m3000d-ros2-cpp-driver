#ifndef OCULUS_APP
#define OCULUS_APP

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

class OculusApp {
public:
  OculusApp();
  ~OculusApp();

private:

  // attributes 
  std::shared_ptr<Logger> console_log_; // logger instance to console

  std::shared_ptr<oculus::SonarDriver> sonar_driver_; // sonar driver instance for interface
  oculus::AsyncService io_service_; // cabled connnection interface utils 

  // methods 
  void Something(); 

};

#endif  // OCULUS_APP
