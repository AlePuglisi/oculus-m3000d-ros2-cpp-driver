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

#ifndef CLIENTCTRL_H
#define CLIENTCTRL_H

#include <QObject>
#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <QAbstractSocket>
#include <QTimer>
#include "../Oculus/Oculus.h"
#include "../Oculus/DataWrapper.h"
#include "../Oculus/OssDataWrapper.h"

class QTcpSocket;


// ----------------------------------------------------------------------------
// OsBufferEntry - contains a return message and an embedded image
class OsBufferEntry
{
public:
  OsBufferEntry();
  ~OsBufferEntry();

  // Methods
  void AddRawToEntry(char* pData, quint64 nData);
  void ProcessRaw(char* pData);

  // Data
  OculusSimplePingResult  m_rfm;         // The fixed length return fire message
  OculusReturnFireMessage m_rff;
  OculusSimplePingResult2 m_rfm2;	
  uchar*                  m_pImage;      // The image data
  short*                  m_pBrgs;       // The bearing table
  QMutex                  m_mutex;       // Lock for buffer accesss

  uint16_t				  m_version;
  quint8*                 m_pRaw;        // The raw data
  quint32                 m_rawSize;     // Size of the raw data record

  bool					  m_simple;
};


class OsClientCtrl;
#define OS_BUFFER_SIZE 10

// ----------------------------------------------------------------------------
// OsReadThread - a worker thread used to read data from the network for the client
class OsReadThread : public QThread
{
  Q_OBJECT

public:
  OsReadThread();
  ~OsReadThread();

  void run() Q_DECL_OVERRIDE;

  void Startup();
  void Shutdown();
  bool IsActive();
  void SetActive(bool active);
  void ProcessRxBuffer();
  void ProcessPayload(char* pData, quint64 nData);

signals:
  void Msg(QString msg);
  void NewReturnFire(OsBufferEntry* pEntry);
  void NewUserConfig(UserConfig config);
  void NotifyConnectionFailed(QString error);

  void socketTimeout();
  void socketReconnected();
  void socketDisconnect();

public:

  // Data
  OsClientCtrl* m_pClient;   // back pointer to the parent client
  bool          m_active;    // Is the run exec active
  QMutex        m_mutex;     // Mutex protection for m_active
  QMutex        m_sending;   // Mutex protection for m_active

  QString       m_hostname;  // The hostname/address of the sonar
  quint16       m_port;      // The port for sonar comms (currently fixed)
  qint32        m_nFlushes;  // Number of times the rx buffer has had to be flushed

  // The raw receive buffer
  char*         m_pRxBuffer; // The rx buffer for incomming data
  qint32        m_nRxMax;    // The maximum size of the rx Buffer
  qint32        m_nRxIn;     // The current amount of unprocessed data in the buffer

  // The recieve buffer for messages
  OsBufferEntry m_osBuffer[OS_BUFFER_SIZE];
  unsigned      m_osInject;   // The position for the next inject

  // The raw send buffer
  QTcpSocket*   m_pSocket;
  char*         m_pToSend;
  qint64        m_nToSend;


   QByteArrayList m_sendBuffer;
   qint64		  m_sendCount;

private slots:

 // void socketError(QAbstractSocket::SocketError error);
  void socketDisconnected();

};


// ----------------------------------------------------------------------------
// ClientCtrl - used to communicate with the oculus sonar
class OsClientCtrl : public QObject
{
  Q_OBJECT

public:
  OsClientCtrl();
  ~OsClientCtrl();

  bool Connect();
  bool Disconnect();
  bool IsOpen();
  void WriteToDataSocket(char* pData, quint16 length);
  void Fire(int mode, double range, double gain, double speedOfSound, double salinity, bool gainAssist, uint8_t gammaCorrection, uint8_t netSpeedLimit);
  void DummyMessage();


  void WriteUserConfig(uint32_t ipAddress, uint32_t ipMask, bool dhcpEnable);
  bool RequestUserConfig();

  bool WaitForReadOrTimeout(uint32_t ms);

public:
  QString     m_hostname;     // The hostname/address of the sonar
  QString     m_mask;

  QMutex      m_lock;
  QWaitCondition m_wait;
  QMutex	m_memLock;

  bool m_received;

  QTimer m_timeout;

	QByteArray	m_readBuffer;

  UserConfig  m_config;       // Oculus user configuration

  OsReadThread  m_readData; // The worker thread for reading data

signals:
  void waitSignal();

  void NewUserConfig();
};

#endif // CLIENTCTRL_H
