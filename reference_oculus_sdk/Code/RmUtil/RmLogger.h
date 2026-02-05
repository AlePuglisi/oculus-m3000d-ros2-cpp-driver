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

#pragma once

#include <QObject>
#include <QFile>

// ----------------------------------------------------------------------------
// The post-ping fire message received back from the sonar
struct RmLogHeader
{
public:
  unsigned       fileHeader;   // Fixed 4 byte header ident type
  unsigned       sizeHeader;   // Size of this structure
  char           source[16];   // 12 character max source identifier
  unsigned short version;      // File version type
  unsigned short encryption;   // Encryption style (0 = none)
  qint64         key;          // Possibly saved encryption key (0 otherwise)
  double         time;         // Time of file creation
};

// ----------------------------------------------------------------------------
// The post-ping fire message received back from the sonar
struct RmLogItem
{
public:
  unsigned       itemHeader;   // Fixed 4 byte header byte
  unsigned       sizeHeader;   // Size of this structure
  unsigned short type;         // Identifer for the contained data type
  unsigned short version;      // Version for the data type
  double         time;         // Time item creation
  unsigned short compression;  // Compression type 0 = none, 1 = qCompress
  unsigned       originalSize; // Size of the payload prior to any compression
  unsigned       payloadSize;  // Size of the following payload
};

// ----------------------------------------------------------------------------
// The current state of the logger
enum elogState
{
  notLogging,
  opening,
  closing,
  logging
};

// ----------------------------------------------------------------------------
// Identify the possible payload types - the logger can accept any payload
// but this record list can be used to allocate type ids to avoid conflicts
enum eRecordTypes
{
	rt_settings          = 1,             // RmSettingsLogger packet
	rt_serialPort        = 2,             // Raw serial string - version contains the port number
	rt_oculusSonar       = 10,            // Raw oculus sonar data
	rt_blueviewSonar     = 11,            // Blueview data log image (raw)
	rt_rawVideo          = 12,            // Raw video logg
	rt_h264Video         = 13,            // H264 compresses video log
	rt_apBattery         = 14,            // ApBattery structure
	rt_apMissionProgress = 15,            // ApMissionProgress structure
	rt_nortekDVL         = 16,            // The complete Nortek DVL structure
	rt_apNavData         = 17,            // ApNavData structures
	rt_apDvlData         = 18,            // ApDvlData structures
	rt_apAhrsData        = 19,            // ApAhrsData structure
	rt_apSonarHeader     = 20,            // ApSonarHeader followed by image
	rt_rawSonarImage     = 21,            // Raw sonar image
	rt_ahrsMtData2       = 22,            // XSens MtData2 message
	rt_apVehicleInfo     = 23,            // Artemis ApVehicleInfo structures
	rt_apMarker          = 24,            // ApMarker structure
	rt_apGeoImageHeader  = 25,            // ApGeoImageHeader
	rt_apGeoImageData    = 26,            // ApGeoImage data of image
	rt_sbgData           = 30,            // SBG compass data message
	rt_ocViewInfo		 = 500			  // Oculus view information
};


// -------------------------------------------------------------------------
// RmSettingsLogger - A log packet that allows the serialization of string data pairs
class RmSettingsLogger
{
public:
  RmSettingsLogger();
  ~RmSettingsLogger();

  void AddValue(QString tag, int     value);
  void AddValue(QString tag, float   value);
  void AddValue(QString tag, double  value);
  void AddValue(QString tag, QString value);
  void AddValue(QString tag, QColor  value);

  quint16 m_nItems;     // Number of items stored in the serialize structure
  quint32 m_size;       // Data payload size
  quint8* m_pData;      // The data payload
};

// ----------------------------------------------------------------------------
// The logger object
class RmLogger : public QObject
{
  Q_OBJECT

public:
  explicit RmLogger(QObject *parent = 0);
  ~RmLogger();

  // Methods
  void      OpenFile(QString filename);
  void      SetupEncryption(quint16 encryption, quint64 key);
  elogState GetLogState();
  bool      LogIsActive();

  // Data
  QString   m_logDir;        // The directory to log files to
  uint32_t  m_logMaxSize;    // Maximum size for a log file
  uint32_t	m_logCurrMaxSize;
  QString   m_ext;           // Extension to use for the log file
  QString   m_fileName;      // Name of the file currently being logged
  QFile     m_file;          // The file
  elogState m_state;         // The state of the logger
  quint16   m_encryption;    // The encryption level to use
  quint64   m_key;           // The encryption key to use
  unsigned  m_nRecords;      // Number of records logged in the current file
  quint64   m_loggedSize;    // Number of bytes logged to the current record

  quint64   m_maxRecords;    // Maximum number of records to log before opening a new file
  quint64   m_maxSize;       // Maximum number of bytes to log before opening a new file

  static const unsigned s_fileHeader;
  static const unsigned s_itemHeader;
  static const char     s_source[16];

signals:

public slots:
  void OpenLog();
  void CloseLog();
  void SetLogDirectory(QString dir);
  void SetMaxLogSize(uint32_t size);
  void LogData(unsigned short type, unsigned short version, bool compress, unsigned size, unsigned char* pData);
};
