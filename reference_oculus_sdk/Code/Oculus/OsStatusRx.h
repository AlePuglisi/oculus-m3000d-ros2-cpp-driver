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

#include <QDateTime>
#include <QObject>
#include "../Oculus/Oculus.h"

class QUdpSocket;

// ----------------------------------------------------------------------------
// Stores the last status message of a sonat witha given id
class COsStatusSonar : public QObject
{
  Q_OBJECT

public:

  COsStatusSonar();

  unsigned        m_id;              // The id of the sonar
  OculusStatusMsg m_osm;             // The last status message associated with this sonar
  QDateTime       m_lastMsgTime;     // The time of the last message
};


// ----------------------------------------------------------------------------
// OsStatusRx - a listening socket for oculus status messages

class OsStatusRx : public QObject
{
    Q_OBJECT

public:
    OsStatusRx();
    ~OsStatusRx();

    void ReadDatagrams();

signals:
    void NewStatusMsg(OculusStatusMsg osm, quint16 valid, quint16 invalid);

public:
    quint16     m_port;       // Port to listen on
    quint16     m_valid;      // Number of valid status messages
    quint16     m_invalid;    // Number of invalid status messages
    QUdpSocket* m_listener;   // Listening socket for status messages
};

