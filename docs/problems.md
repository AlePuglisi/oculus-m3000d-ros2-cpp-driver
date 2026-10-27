# Problems and Possible Improvements

List of known software/hardware problems, bugs and ideas for further improvements.

## Problems and bugs to solve

**Changing the gain in high-frequency mode**
: You may be surprised when reconfiguring the gain in high-frequency mode: the number you set will not be
  the one reflected in the ROS2 parameter. This is what the `SonarDriver` source code of the low-level
  `oculus_driver_lib` states:

  ```cpp
  // When masterMode = 2, the sonar force gainPercent between 40% and
  // 100%, BUT still needs requested gainPercent to be between 0%
  // and 100%. (If you request a gainPercent=0 in masterMode=2, the
  // fireMessage in the ping results will be 40%). The gainPercent is
  // rescaled here to ensure consistent parameter handling on client
  // side).
  if(newConfig.masterMode == 2) {
      newConfig.gainPercent = (newConfig.gainPercent - 40.0) * 100.0 / 60.0;
  }
  ```

  As you can see, a proper value rescaling is performed when sending the request to the sonar.
  The behavior without this rescaling has not been tested yet.

**Reconfiguring the sensor publish rate**
: From the last tests, the sensor publish rate looks limited to a maximum of 7 fps, even when
  reconfiguring it to a higher rate. This is a relevant problem that must be solved to operate the sonar
  efficiently in the field.

**Running sonar + IP camera**
: When the sonar and an IP camera (Sony) run simultaneously, they seem to overload the Ethernet
  communication, and multiple sonar frames are lost periodically. This is a relevant issue to address in
  order to deploy both camera and sonar in the field.

## To test

**Sonar + USB camera**
: Does the same problem as with the IP camera occur?

## To do and ideas

**Implement robust sonar image filtering**
: Before implementing any perception algorithm, it is mandatory to find optimal ways to filter as much
  noise as possible from the sonar data. Artifacts and speckle noise will influence and degrade the
  performance of any computer vision and deep learning technique.

**Ensure bandwidth**
: Ensure that the communication can work at 10 Mbps and not only at 100 Mbps. ROS2 provides a CLI tool
  for this:

  ```bash
  ros2 topic bw /your/topic_name
  ```

  By default, this tool outputs the data rate in MB/s (megabytes per second). Multiply by 8 to get Mbps.
