/******************************************************************************
 * (c) Copyright 2017 Blueprint Subsea.
 * This file is part of Oculus Viewer
 *
 * Oculus Viewer is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Oculus Viewer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *****************************************************************************/

#ifndef OCULUS_H
#define OCULUS_H

#include <stdint.h>

// All structures are single byte packed
#pragma pack(push, 1)

// The test id contained in the oculus header file
#define OCULUS_CHECK_ID 0x4f53

enum OculusMasterStatusType : uint8_t
{
    oculusMasterStatusSsblBoot,
    oculusMasterStatusSsblRun,
    oculusMasterStatusMainBoot,
    oculusMasterStatusMainRun,
};

enum OculusPauseReasonType : uint8_t
{
    oculusPauseMagSwitch,
    oculusPauseBootFromMain,
    oculusPauseFlashError,
    oculusPauseJtagLoad,
};

enum OculusTemperatureStatusType : uint8_t
{
    oculusTempGood,
    oculusTempOverheat,
    oculusTempReserved,
    oculusTempOvermax,
};

// -----------------------------------------------------------------------------
enum OculusDeviceType : uint16_t
{
    deviceTypeUndefined = 0,
    deviceTypeImagingSonar = 1,
};

// -----------------------------------------------------------------------------
enum OculusMessageType : uint16_t
{
    messageSimpleFire = 0x15,
    messagePingResult = 0x22,
    messageSimplePingResult = 0x23,
    messageUserConfig = 0x55,
    messageDummy = 0xff,
};

enum PingRateType : uint8_t
{
    pingRateNormal = 0x00, // 10Hz max ping rate
    pingRateHigh = 0x01, // 15Hz max ping rate
    pingRateHighest = 0x02, // 40Hz max ping rate
    pingRateLow = 0x03, // 5Hz max ping rate
    pingRateLowest = 0x04, // 2Hz max ping rate
    pingRateStandby = 0x05, // Disable ping
};

// -----------------------------------------------------------------------------
enum DataSizeType : uint8_t
{
    dataSize8Bit,
    dataSize16Bit,
    dataSize24Bit,
    dataSize32Bit,
};

// -----------------------------------------------------------------------------
enum OculusPartNumberType : uint16_t
{
    partNumberUndefined = 0,

    partNumberM370s = 1041,
    partNumberMT370s = 2418,
    partNumberMD370s = 1433,
    partNumberMD370s_Burton = 2294,
    partNumberMD370s_Impulse = 1217,

    partNumberC550d = 4713,

    partNumberM750d = 1032,
    partNumberMT750d = 2419,
    partNumberMD750d = 1434,
    partNumberMD750d_Burton = 1921,
    partNumberMD750d_Impulse = 1244,

    partNumberM1200d = 1042,
    partNumberMT1200d = 2420,
    partNumberMD1200d = 1435,
    partNumberMD1200d_Burton = 2086,
    partNumberMD1200d_Impulse = 1219,

    partNumberM3000d = 2203,
    partNumberMT3000d = 2599,
    partNumberMD3000d_Burton = 2659,
    partNumberMD3000d_Impulse = 2658,

    partNumberEnd = 0xFFFF
};

struct OculusMessageHeader
{
    uint16_t oculusId; // Fixed ID 0x4f53
    uint16_t srcDeviceId; // The device id of the source
    uint16_t dstDeviceId; // The device id of the destination
    uint16_t msgId; // Message identifier
    uint16_t msgVersion;
    uint32_t payloadSize; // The size of the payload (header not included)
    uint16_t partNumber;
};

// -----------------------------------------------------------------------------
struct OculusSimpleFireMessage
{
    OculusMessageHeader head;

    uint8_t masterMode;
    PingRateType pingRate;
    uint8_t networkSpeed;
    uint8_t gammaCorrection;
    uint8_t flags;
    double range;
    double gainPercent;
    double speedOfSound;
    double salinity;
};

struct OculusSimpleFireMessage2
{
    OculusMessageHeader head;

    uint8_t masterMode;
    PingRateType pingRate;
    uint8_t networkSpeed;
    uint8_t gammaCorrection;
    uint8_t flags;
    double range;
    double gainPercent;
    double speedOfSound;
    double salinity;
    uint32_t extFlags;
    uint32_t reserved0[2];
    uint32_t beaconLocatorFrequency;
    uint32_t reserved1[5];
};

// -----------------------------------------------------------------------------
struct OculusSimplePingResult
{
    OculusSimpleFireMessage fireMessage;
    uint32_t pingId; 			/* An incrementing number */
    uint32_t status;
    double frequency;				/* The acoustic frequency (Hz) */
    double temperature;				/* The external temperature (deg C) */
    double pressure;				/* The external pressure (bar) */
    double speedOfSoundUsed; /* The actual used speed of sound (m/s). May be
                                different to the speed of sound set in the fire
                                message */
    uint32_t pingStartTime;
    DataSizeType dataSize; 			/* The size of the individual data entries */
    double rangeResolution;			/* The range in metres corresponding to a single range line */
    uint16_t nRanges;			/* The number of range lines in the image*/
    uint16_t nBeams;			/* The number of bearings in the image */
    uint32_t imageOffset; 		/* The offset in bytes of the image data from the start of the network message */
    uint32_t imageSize; 		/* The size in bytes of the image data */
    uint32_t messageSize; 		/* The total size in bytes of the network message */
    // *** NOT ADDITIONAL VARIABLES BEYOND THIS POINT ***
    // There will be an array of bearings (shorts) found at the end of the message structure
    // Allocated at run time
    // short bearings[];
    // The bearings to each of the beams in 0.01 degree resolution
};

struct OculusSimplePingResult2
{
    OculusSimpleFireMessage2 fireMessage;
    uint32_t pingId; /* An incrementing number */
    uint32_t status;
    double frequency; /* The acoustic frequency (Hz) */
    double temperature; /* The external temperature (deg C) */
    double pressure; /* The external pressure (bar) */
    double heading; /* The heading (degrees) */
    double pitch; /* The pitch (degrees) */
    double roll; /* The roll (degrees) */
    double speedOfSoundUsed; /* The actual used speed of sound (m/s) */
    double pingStartTime; /* In seconds from sonar powerup (to microsecond
                             resolution) */
    DataSizeType dataSize; /* The size of the individual data entries */
    double rangeResolution; /* The range in metres corresponding to a single
                               range line */
    uint16_t nRanges; /* The number of range lines in the image*/
    uint16_t nBeams; /* The number of bearings in the image */
    uint32_t spare0;
    uint32_t spare1;
    uint32_t spare2;
    uint32_t spare3;
    uint32_t
        imageOffset; /* The offset in bytes of the image data from the start */
    uint32_t imageSize; /* The size in bytes of the image data */
    uint32_t messageSize; /* The total size in bytes of the network message */
    // short bearings[]; /* The brgs of the formed beams in 0.01 degree
    // resolution */
};

// -----------------------------------------------------------------------------
struct OculusVersionInfo
{
    uint32_t firmwareVersion0; /* The arm0 firmware version major(8 bits),
                                  minor(8 bits), build (16 bits) */
    uint32_t firmwareDate0; /* The arm0 firmware date */
    uint32_t firmwareVersion1; /* The arm1 firmware version major(8 bits),
                                  minor(8 bits), build (16 bits) */
    uint32_t firmwareDate1; /* The arm1 firmware date */
    uint32_t firmwareVersion2; /* The bitfile version */
    uint32_t firmwareDate2; /* The bitfile date */
};

// -----------------------------------------------------------------------------
struct OculusStatusMsg
{
    OculusMessageHeader hdr;

    uint32_t deviceId;
    OculusDeviceType deviceType;
    OculusPartNumberType partNumber;
    uint32_t status;
    OculusVersionInfo versionInfo;
    uint32_t ipAddr;
    uint32_t ipMask;
    uint32_t connectedIpAddr;
    uint8_t macAddr0;
    uint8_t macAddr1;
    uint8_t macAddr2;
    uint8_t macAddr3;
    uint8_t macAddr4;
    uint8_t macAddr5;
    double temperature0;
    double temperature1;
    double temperature2;
    double temperature3;
    double temperature4;
    double temperature5;
    double temperature6;
    double temperature7;
    double pressure;
};

struct OculusUserConfig
{
    uint32_t ipAddr;
    uint32_t ipMask;
    uint32_t dhcpEnable;
};

struct OculusUserConfigMessage {
	OculusMessageHeader head;
	OculusUserConfig config;
};

struct PingConfig
{
    uint8_t b0;
    double d0;
    double range;
    double d2;
    double d3;
    double d4;
    double d5;
    double d6;
    uint16_t nBeams;
    double d7;
    uint8_t b1;
    uint8_t b2;
    uint8_t b3;
    uint8_t b4;
    uint8_t b5;
    uint8_t b6;
    uint16_t u0;
    uint8_t b7;
    uint8_t b8;
    uint8_t b9;
    uint8_t b10;
    uint8_t b11;
    uint8_t b12;
    uint8_t b13;
    uint8_t b14;
    uint8_t b15;
    uint8_t b16;
    uint16_t u1;
};

struct s0
{
    uint8_t b0;
    double d0;
    uint16_t u0;
    uint16_t u1;
};

struct s2
{
    uint8_t b0;
};

struct s7
{
    uint8_t b0;
    uint8_t b1;
};

struct s9
{
    int i0;
    int i1;
    int i2;
    int i3;
    int i4;
    int i5;
};

struct s8
{
    uint8_t b0;
    double d0;
    double d1;
};

struct s1
{
    uint8_t b0;
    uint16_t u0;
    uint8_t b1;
    double d0;
};

struct s3
{
    uint8_t b0;
    uint8_t b1;
    uint8_t b2;
    uint8_t b3;
    uint8_t b4;
    uint8_t b5;
    uint8_t b6;
    uint8_t b7;
    uint8_t b8;
    uint8_t b9;
    uint8_t b10;
    uint8_t b11;
    uint8_t b12;
    uint8_t b13;
    uint8_t b14;
    uint8_t b15;
    uint16_t u0;
    uint8_t b16;
    double d0;
    double d1;
};

struct s4
{
    uint8_t b0;
    uint8_t b1;
    double d0;
    double d1;
    double d2;
};

struct s5
{
    uint8_t b0;
    uint8_t b1;
    uint16_t u0;
    uint16_t u1;
    uint16_t u2;
    uint16_t u3;
    uint16_t u4;
    uint16_t u5;
};

struct s6
{
    double d0;
    double d1;
};

struct s10
{
    int i0;
    int i1;
    int i2;
    int i3;
};

struct s11
{
    double d0;
    double d1;
    double d2;
    double d3;
    double d4;
};

struct s12
{
    double d0;
    double d1;
    double d2;
    double d3;
    double d4;
    double d5;
    double d6;
};

struct PingParameters
{
    uint32_t u0;
    uint32_t u1;
    double d1;
    double d2;
    uint32_t u2;
    uint32_t u3;

    double d3;
    double d4;
    double d5;
    double d6;
    double d7;
    double d8;
    double d9;
    double d10;
    double d11;
    double d12;
    double d13;
    double d14;
    double d15;
    double d16;
    double d17;
    double d18;
    double d19;
    double d20;

    uint32_t u4;
    uint32_t nRangeLinesBfm;
    uint16_t u5;
    uint16_t u6;
    uint16_t u7;
    uint32_t u8;
    uint32_t u9;
    uint8_t b0;
    uint8_t b1;
    uint8_t b2;
    uint32_t imageOffset; /* The offset in bytes of the image data (CHN, CQI,
                           BQI or BMG) from the start of the buffer */
    uint32_t imageSize; /* The size in bytes of the image data (CHN, CQI, BQI or
                           BMG) */
    uint32_t messageSize; // The total size in bytes of the network message
    // *** NOT ADDITIONAL VARIABLES BEYOND THIS POINT ***
    // There will be an array of bearings (shorts) found at the end of the
    // message structure Allocated at run time short bearings[]; The bearings to
    // each of the beams in 0.01 degree resolution
};

struct OculusReturnFireMessage
{
    OculusMessageHeader head;
    PingConfig ping;
    s0 t0;
    s1 t1;
    s2 t2;
    s3 t3;
    s4 t4;
    s5 t5;
    s6 t6;
    s7 t7;
    s8 t8;
    s9 t9;
    s10 t10;
    s11 t11;
    s12 t12;
    PingParameters ping_params;
};

// Oculus configuration information
struct OculusInfo
{
    // Part number for which the info relates to
    OculusPartNumberType partNumber;
    // Has a low-frequency mode
    bool hasLF;
    // Maximum low-frequency range
    double maxLF;
    // Has a high-frequency mode
    bool hasHF;
    // Maximum high-frequency range
    double maxHF;
    // Description
    char* model;
};

#pragma pack(pop)

// Sonar configuration list. Defines the maximum ranges for various part numbers
// If the part number is not in this list, it defaults to the "partNumberUndefined"
// configuration
const OculusInfo OculusSonarInfo[] = {
    // All undefined sonars and sonars not in this list
    {partNumberUndefined,
     // Supports LF mode
     true,
     // Up to 120m range
     120,
     // Supports HF mode
     true,
     // Up to 40m range
     40},
    // -------------------------------------------------------------------------
    // M370
    {partNumberM370s,
     // Supports LF mode
     true,
     // Up to 200m range
     200,
     // No HF mode
     false,
     // Up to 40m range
     -1},
    // M370s_Deep
    {partNumberMD370s,
     // Supports LF mode
     true,
     // Up to 200m range
     200,
     // No HF mode
     false,
     -1},
    // -------------------------------------------------------------------------
    // C550d
    {partNumberC550d,
     // Supports LF mode
     true,
     // Up to 100m range
     100,
     // Supports HF mode
     true,
     // Up to 30m range
     30},
    // -------------------------------------------------------------------------
    // M750d
    {partNumberM750d,
     // Supports LF mode
     true,
     // Up to 120m range
     120,
     // Supports HF mode
     true,
     // Up to 40m range
     40},
    {partNumberMD750d,
     // Supports LF mode
     true,
     // Up to 120m range
     120,
     // Supports HF mode
     true,
     // Up to 40m range
     40},
    // -------------------------------------------------------------------------
    // M1200d
    {partNumberM1200d,
     // Supports LF mode
     true,
     // Up to 40m range
     40,
     // Supports HF mode
     true,
     // Up to 10m range
     10},
    {partNumberMD1200d,
     // Supports LF mode
     true,
     // Up to 40m range
     40,
     // Supports HF mode
     true,
     // Up to 10m range
     10},
    // -------------------------------------------------------------------------
    // M3000d
    {partNumberM3000d,
     // Supports LF mode
     true,
     // Up to 30m range
     30,
     // Supports HF mode
     true,
     // Up to 5m range
     5},
    // End of the list
    {partNumberEnd}};

#endif // OCULUS_H
