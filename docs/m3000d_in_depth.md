# Oculus M3000d in Depth

More detailed hardware and software information to use, modify or improve the M3000d driver.

## About the M3000d hardware

This section provides some useful details on the Oculus M3000d hardware. Some of these details can be
relevant for sensor fusion algorithms or to better understand the available data and processing steps.

## Integrated proprioceptive sensors (attitude, heading and pressure)

In addition to the transducers used to generate the acoustic image (transmit sound and record echo),
the sonar integrates additional proprioceptive sensors.

In particular, the M3000d integrates an Attitude and Heading Reference System (AHRS) and a pressure
sensor (sensor models and specifications are not available). The images below show the location and
axis configuration of the AHRS (shared by the Blueprint Subsea technical support team).

```{figure} assets/images/M3000d_AHRS_location.png
:width: 480px
:alt: AHRS location

AHRS location.
```

```{figure} assets/images/M3000d_AHRS_axis.png
:width: 480px
:alt: AHRS axis configuration

AHRS axis configuration.
```

:::{important}
The raw data of the IMU used by the AHRS are not exposed by the SDK.
Roll, pitch and heading are stored in the Oculus ping message. From some preliminary tests these data
are not reliable, and calibration is not possible.
:::

With respect to the pressure information, additional tests are required to estimate its quantitative
performance. The **pressure** data is also stored in the Oculus ping message, in **Pa**. After calibrating
the sensor for the static bias and filtering the noise, it should provide accurate pressure measurements
for sonar depth.
