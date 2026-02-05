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

#include "osstatusrx.h"
#include <QUdpSocket>

// ----------------------------------------------------------------------------
// OsStatusRx - a listening socket for oculus status messages

OsStatusRx::OsStatusRx()
{
  // Create and setup a broadcast listening socket
  m_listener = new QUdpSocket(this);
  m_port     = 52102;   // fixed port for status messages
  m_valid    = 0;
  m_invalid  = 0;

  qDebug() << QString("CONNECTING STATUS SOCKET");
  // Connect the data signal
  connect(m_listener, &QUdpSocket::readyRead, this, &OsStatusRx::ReadDatagrams);

  // Bind the socket (added Reuse address hint as this seems to allow other instances
  // of Oculus Viewer to see Sonars)

  m_listener->bind(m_port, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
}

OsStatusRx::~OsStatusRx()
{

}

// ----------------------------------------------------------------------------
// Signalled when there is data available in the socket buffer
// Note that if the Oculus Viewer software is running on a PC with two network ports then it is
// possible that both these ports will receive the status message
// In this case we will see twice as many status messages as expected
void OsStatusRx::ReadDatagrams()
{
  // Read through any available datagrams
  while (m_listener->hasPendingDatagrams())
  {           
    // Read the datagram out of the socket buffer
    QByteArray datagram;

    datagram.resize(m_listener->pendingDatagramSize());
    m_listener->readDatagram(datagram.data(), datagram.size());

    // If datagra is of correct size, cast and signal any observers
    if (datagram.size() == sizeof(OculusStatusMsg))
    {
      OculusStatusMsg osm;
      memcpy(&osm, datagram.data(), datagram.size());

      if (osm.hdr.oculusId == OCULUS_CHECK_ID)
      {
        m_valid++;

        emit NewStatusMsg(osm, m_valid, m_invalid);
      }
    }
    else
    {        
      m_invalid++;
    }
  }
}


