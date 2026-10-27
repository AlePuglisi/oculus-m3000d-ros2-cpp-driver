# Sonar Data Brief Description

Preliminary notes on how the Oculus M3000d sonar generates the acoustic image readings and how they are
stored in the ROS2 driver. To read more about the data structure definitions, see the official
[documentation file](./assets/NT-148-D00527-06_Oculus_Data_Structure_Definitions.pdf).

## Oculus network communication

As described in the SDK
[documentation file](./assets/NT-148-D00527-06_Oculus_Data_Structure_Definitions.pdf),
the sensor communicates with the processor (computer or embedded system) over both UDP and TCP.

**UDP socket (port 52102)**
: Used for the status datagram message at 1 Hz. Stores the sensor IP and health information.
  The low-level `oculus_driver` uses the UDP port to retrieve the IP and establish the TCP connection.
  This is done once the driver class is initialized in the `oculus_sonar_node` setup.

**TCP socket (port 52100)**
: Channel for control signals and sensor data.
  The sonar pings for a limited amount of time only, so a new request has to be transmitted periodically
  to keep the imaging alive at the requested `pingRate`.

## Oculus SDK messages (relevant ones)

To see the full source code of the proprietary Oculus messages, explore the message definitions in
[Oculus.h](./../oculus_driver_lib/include/oculus_driver/Oculus.h)
and the wrappers in
[OculusMessage.h](./../oculus_driver_lib/include/oculus_driver/OculusMessage.h).

## Oculus ROS2 messages (relevant ones)

To see the full source code of the custom Oculus ROS2 messages, explore the `msg` definitions in the
[oculus_interfaces](./../OCULUS_ROS2/oculus_interfaces)
package.

- [`Ping`](./../OCULUS_ROS2/oculus_interfaces/msg/Ping.msg): complete information about the sonar ping return, including the raw acoustic return matrix.
- [`OculusPing2`](./../OCULUS_ROS2/oculus_interfaces/msg/OculusPing2.msg): *(description to be added)*
- [`OculusStatus`](./../OCULUS_ROS2/oculus_interfaces/msg/OculusStatus.msg): *(description to be added)*

## Acoustic image formation process

General, high-level description of the logic behind the acoustic image formation process.

:::{note}
This section is still empty in the original README.
:::

### Oculus sonar logic

Acoustic image formation process for the **Oculus Blueprint M3000d** series sonar. The same logic is used
by previous versions of the M-series forward-looking sonar, with some changes in the software
implementation in the SDK. (Other forward-looking sonars probably use similar workflows.)
