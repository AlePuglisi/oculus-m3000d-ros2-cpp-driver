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
// A tool for playing files in the .log format as logged by RmLogger

class RmPlayer : public QObject
{
  Q_OBJECT

public:
  RmPlayer();
  ~RmPlayer();

  // Methods
  bool OpenFile(QString file);
  void CloseFile();
  void Seek(quint64 pos);
  bool ReadFailed(QString reason);
  bool OpenFileAt(QString file, quint64 pos);
  int  CreateTypeIndex(QString file, int type, quint64** nEntries);

  // Data
  QFile   m_file;            // The file
  QString m_fileName;        // The name of the last file opened
  quint64 m_fileSize;        // The size of the file being read
  quint64 m_totalBytes;      // The amount of data read so far from the file
  quint8* m_pPayloadBuffer;  // Byte buffer to store the payload in


  bool    m_repeat;

signals:
  void NewPayload(unsigned short type, unsigned short version, double time, unsigned payloadSize, quint8* pPayload);
  void NewPayloadPos(unsigned short type, unsigned short version, double time, unsigned payloadSize, quint8* pPayload, quint64 pos);
  void EndOfFile();

public slots:
  bool ReadNextItem();
  bool PeekNextItem(double *time);
  bool ReadNextItemOfType(quint16 type);
};

