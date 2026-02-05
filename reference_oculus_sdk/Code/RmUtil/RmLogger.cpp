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

#include <QDateTime>
#include <QFileDialog>
#include <QStandardPaths>
#include <QDebug>
#include <QFile>

#include "RmLogger.h"

#include <QMessageBox>

const unsigned RmLogger::s_fileHeader = 0x11223344;               // Something endian
const unsigned RmLogger::s_itemHeader = 0xaabbccdd;
const char     RmLogger::s_source[16] = "Oculus";

RmLogger::RmLogger(QObject *parent) : QObject(parent)
{
  m_state      = notLogging;
  m_encryption = 0;
  m_key        = 0;
  m_nRecords   = 0;
  m_loggedSize = 0;
	m_ext        = ".oculus";

  // Initialise the log directory documents location
  m_logDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
}

RmLogger::~RmLogger()
{
  if (m_state == logging)
    CloseLog();
}

// ----------------------------------------------------------------------------
// Open the given filename
void RmLogger::OpenFile(QString filename)
{
  QDateTime dt = QDateTime::currentDateTime();

  // Configure the file header
  m_state = opening;

  m_fileName = filename;

  RmLogHeader header;
  memset(&header, 0, sizeof(RmLogHeader));

  header.fileHeader = s_fileHeader;
  header.sizeHeader = sizeof(RmLogHeader);
  memcpy(header.source, s_source, 16);
  header.version    = 1;
  header.encryption = 0;
  header.time       = (double) dt.toMSecsSinceEpoch() / 1000.0;

  // Reset the log counters
  m_nRecords   = 0;
  m_loggedSize = 0;

  m_file.setFileName(m_fileName);

  if (m_file.open(QIODevice::WriteOnly))
  {
    // Write the file header
    m_file.write((char*)&header, sizeof(RmLogHeader));
    m_loggedSize += sizeof(RmLogHeader);

    m_state = logging;
  }
  else
  {
    m_state = notLogging;
  }
}

// ----------------------------------------------------------------------------
// Setup the encryption level for logging/replay
void RmLogger::SetupEncryption(quint16 encryption, quint64 key)
{
  m_encryption = encryption;
  m_key        = key;
}

// ----------------------------------------------------------------------------
// Return the current logging state
elogState RmLogger::GetLogState()
{
  return m_state;
}

// ----------------------------------------------------------------------------
// Convienince function for log testing
bool RmLogger::LogIsActive()
{
  return (m_state == logging);
}

// ----------------------------------------------------------------------------
// Browse for the directory to log to
void RmLogger::SetLogDirectory(QString logDir)
{
  m_logDir = logDir;

  //qDebug() << "The log directory is set to:" + logDir;
}

// ----------------------------------------------------------------------------
void RmLogger::SetMaxLogSize(uint32_t size)
{
  m_logMaxSize = (size * 1048576);

  //qDebug() << "Maximum log size:" + size;
}

// ----------------------------------------------------------------------------
// Open the logging stream and make it available for log requests
void RmLogger::OpenLog()
{
  QDateTime dt = QDateTime::currentDateTime();

  m_logCurrMaxSize = m_logMaxSize;

  QString filename = m_logDir + QDir::separator() + QString(s_source) + dt.toString("_yyyyMMdd_hhmmss") + m_ext;

  // Create the log directory if it doesn't exist
  QDir ld(m_logDir);

  // If the log directory doesn't exist, create it
  if (!ld.exists(m_logDir)) {
	  QMessageBox msg;
	  msg.setIcon(QMessageBox::Critical);
	  msg.setStandardButtons(QMessageBox::Ok);

	  QString str = "Unable to start logging.\r\n\r\nThe log directory does not exist:\r\n\r\n";
	  str += m_logDir;

	  msg.setText(str);
	  msg.setWindowTitle("Logging Error");
	  msg.exec();

	  return;
  }

  OpenFile(filename);
}


// ----------------------------------------------------------------------------
// Shut down the current logging stream
void RmLogger::CloseLog()
{
  if (m_state == logging)
  {
    m_state = closing;

    m_file.close();
  }

  m_state = notLogging;
}


// ----------------------------------------------------------------------------
// (SLOT) Principal logging command
void RmLogger::LogData(unsigned short type, unsigned short version, bool compress, unsigned size, unsigned char* pData)
{
  // Check whether we've exceeded the maximum log size
  if (m_state == logging) {
	  if ((m_logCurrMaxSize > 0) && ((uint32_t)m_file.size() > m_logCurrMaxSize)) {
         // Close the existing log file
         CloseLog();
         // Open a new log file
         OpenLog();
      }
  }

  // If we are logging then add the new data into the stream
  if (m_state == logging && m_file.isOpen())
  {
    // Prepare the header
    RmLogItem logItem;
    memset(&logItem, 0, sizeof(RmLogItem));

    logItem.itemHeader   = s_itemHeader;
    logItem.sizeHeader   = sizeof(RmLogItem);
    logItem.type         = type;
    logItem.version      = version;
    logItem.time         = (double) QDateTime::currentDateTime().toMSecsSinceEpoch() / 1000.0;
    logItem.originalSize = size;

    if (compress == false)
    {
      logItem.compression = false;
      logItem.payloadSize = size;

      // Log the item header
      m_file.write((char*)&logItem, sizeof(RmLogItem));
      m_loggedSize += sizeof(RmLogItem);

      // Write the payload
      m_file.write((char*)pData, size);
      m_loggedSize += size;

      m_nRecords++;
    }
    else
    // Class 1 compression - use the Qt compression alg
    {
      // Compress the image (Qt default compression)
      QByteArray compressed = qCompress(pData, size, 1);

      logItem.compression  = 1;  // 1 = qt compression
      logItem.payloadSize  = compressed.size();

      // Log the item header
      m_file.write((char*)&logItem, sizeof(RmLogItem));
      m_loggedSize += sizeof(RmLogItem);

      // Write the payload
      m_file.write((char*)compressed.data(), compressed.size());
      m_loggedSize += compressed.size();

      m_nRecords++;

      //qDebug() << "Compressed data  Original: " + QString::number((double)logItem.originalSize / 1024.0) + "kb Compressed: " + QString::number((double)logItem.payloadSize / 1024.0) + " " + QString::number(100.0 * (double)logItem.payloadSize / (double) logItem.originalSize) + "%";
    }
  }
}

// ============================================================================
// RmSettingsLogger - A log packet that allows the serialization of string data pairs

RmSettingsLogger::RmSettingsLogger()
{
  m_nItems = 0;
  m_size   = 0;
  m_pData  = nullptr;
}

RmSettingsLogger::~RmSettingsLogger()
{
  if (m_pData)
    delete m_pData;
}

// ----------------------------------------------------------------------------
// Add an integer value to the data packet
void RmSettingsLogger::AddValue(QString tag, int value)
{
  Q_UNUSED(tag)
  Q_UNUSED(value)
}

// ----------------------------------------------------------------------------
// Add a float value to the data packet
void RmSettingsLogger::AddValue(QString tag, float value)
{
  Q_UNUSED(tag)
  Q_UNUSED(value)
}

// ----------------------------------------------------------------------------
// Add a double value to the data packet
void RmSettingsLogger::AddValue(QString tag, double value)
{
  Q_UNUSED(tag)
  Q_UNUSED(value)
}

// ----------------------------------------------------------------------------
// Add a QString value to the data packet
void RmSettingsLogger::AddValue(QString tag, QString value)
{
  Q_UNUSED(tag)
  Q_UNUSED(value)
}

// ----------------------------------------------------------------------------
// Add a QColor value to the data packet
void RmSettingsLogger::AddValue(QString tag, QColor value)
{
  Q_UNUSED(tag)
  Q_UNUSED(value)
}
