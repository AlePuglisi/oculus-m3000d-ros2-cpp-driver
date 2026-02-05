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

#include <QDebug>

#include "RmPlayer.h"
#include "RmLogger.h"

RmPlayer::RmPlayer()
{
  m_pPayloadBuffer = nullptr;
  m_fileSize       = 0;

  m_repeat = false;
}

RmPlayer::~RmPlayer()
{
  if (m_pPayloadBuffer)
    delete m_pPayloadBuffer;

  m_pPayloadBuffer = nullptr;
}

//   void NewPayload(unsigned short type, unsigned short version, double time, unsigned payloadSize, quint8* pPayload);

// ----------------------------------------------------------------------------
// Read through a file, perform any needed uncompression or decryption and emit the
// payloads as a signal
bool RmPlayer::OpenFile(QString file)
{
  // Keep the last file name
  m_fileName = file;

  m_file.setFileName(m_fileName);

  if (m_file.open(QIODevice::ReadOnly))
  {
    m_fileSize = m_file.size();

    if (m_fileSize < sizeof(RmLogHeader))
      return ReadFailed("File is not big enough to contain a header");

    // Check the file header
    RmLogHeader header;

    m_totalBytes = m_file.read((char*)&header, sizeof(RmLogHeader));

    if (m_totalBytes != sizeof(RmLogHeader))
      return ReadFailed("Cannot read the file header");

    if (header.fileHeader != RmLogger::s_fileHeader)
      return ReadFailed("Not a valid header");

    if (header.sizeHeader != sizeof(RmLogHeader))
      return ReadFailed("File header version mismatch");

    return true;
  }
  else
    return false;
}

// ----------------------------------------------------------------------------
// Close the file
void RmPlayer::CloseFile()
{
  m_file.close();
}

// ----------------------------------------------------------------------------
void RmPlayer::Seek(quint64 pos)
{
  if (m_file.isOpen())
  {
    m_file.seek(pos);
    m_totalBytes = pos;
  }
}

// ----------------------------------------------------------------------------
// Clean up the file state and exit
bool RmPlayer::ReadFailed(QString reason)
{
  //qDebug() << reason;

  m_file.close();

  if (m_pPayloadBuffer)
    delete m_pPayloadBuffer;

  m_pPayloadBuffer = nullptr;

  return false;
}

// ----------------------------------------------------------------------------
// Open a file and seek to the given file position - push out one record
bool RmPlayer::OpenFileAt(QString file, quint64 pos)
{
  if (OpenFile(file))
    return m_file.seek(pos);
  else
    return false;
}

// ----------------------------------------------------------------------------
// Create an index list of all the entries of a given type
int RmPlayer::CreateTypeIndex(QString file, int type, quint64** ppEntries)
{
  int nEntries = 0;

  if (OpenFile(file))
  {
    quint64 bytesLeft = m_fileSize - m_totalBytes;

    while (bytesLeft > sizeof(RmLogItem))
    {
      m_file.seek(m_totalBytes);

      RmLogItem item;
      quint64 readBytes = m_file.read((char*)&item, sizeof(RmLogItem));

      if (readBytes != sizeof(RmLogItem))
        break;

      // Record the position of the data record
      quint64 filePos = m_totalBytes;

      // Update total bytes read
      m_totalBytes += readBytes;

      if (item.itemHeader != RmLogger::s_itemHeader)
        break;

      if (item.sizeHeader != sizeof(RmLogItem))
        break;

      // If we are the type then add to the index list
	  if (item.type == type)
      {
        nEntries++;
        *ppEntries = (quint64*) realloc(*ppEntries, nEntries * sizeof(quint64));
        (*ppEntries)[nEntries - 1] = filePos;
      }

      // Seek past in the payload
      m_totalBytes += item.payloadSize;

      bytesLeft = m_fileSize - m_totalBytes;
    }

    CloseFile();

    return nEntries;
  }
  else
    return 0;
}

// ----------------------------------------------------------------------------
// Read and broadcast an item from the current file
bool RmPlayer::ReadNextItem()
{
  if (!m_file.isOpen())
    return false;

  quint64 bytesLeft = m_fileSize - m_totalBytes;

  if (bytesLeft == 0)
  {
    emit EndOfFile();
    return false;
  }

  if (bytesLeft < sizeof(RmLogItem))
    return ReadFailed("Incomplete item header found. FS:" + QString::number(m_fileSize) + " BR:" + QString::number(m_totalBytes));

  RmLogItem item;

  quint64 readBytes = m_file.read((char*)&item, sizeof(RmLogItem));

  if (readBytes != sizeof(RmLogItem))
    return ReadFailed("Cannot read the item header");

  // Record the position of the data record
  quint64 filePos = m_totalBytes;

  // Update total bytes read
  m_totalBytes += readBytes;

  if (item.itemHeader != RmLogger::s_itemHeader)
    return ReadFailed("Not a valid item header");

  if (item.sizeHeader != sizeof(RmLogItem))
    return ReadFailed("Item header version mismatch");

  // Make sure there is enough room in the buffer
  m_pPayloadBuffer = (quint8*) realloc (m_pPayloadBuffer, item.payloadSize);

  // Try to read in the payload
  readBytes = m_file.read((char*)m_pPayloadBuffer, item.payloadSize);

  if (readBytes != item.payloadSize)
    return ReadFailed("Unable to read item payload");

  // Update total bytes read
  m_totalBytes += item.payloadSize;

  // Emit the new data packet for processing
  emit NewPayload(item.type, item.version, item.time, item.payloadSize, m_pPayloadBuffer);

  // Emit 2nd signal with file posthe new data packet for processing
  emit NewPayloadPos(item.type, item.version, item.time, item.payloadSize, m_pPayloadBuffer, filePos);


  return true;
}


// -----------------------------------------------------------------------------
bool RmPlayer::PeekNextItem(double *time) {
	if (!m_file.isOpen())
	  return false;
/*
	quint64 bytesLeft = m_fileSize - m_totalBytes;

	if (bytesLeft == 0)
	{
	  emit EndOfFile();
	  return false;
	}

	if (bytesLeft < sizeof(RmLogItem))
	  return ReadFailed("Incomplete item header found. FS:" + QString::number(m_fileSize) + " BR:" + QString::number(m_totalBytes));

	RmLogItem item;

	quint64 readBytes = m_file.read((char*)&item, sizeof(RmLogItem));

	if (readBytes != sizeof(RmLogItem))
	  return ReadFailed("Cannot read the item header");
*/

	// ###########################
	quint64 totalBytes = m_totalBytes;
	quint64 bytesLeft = m_fileSize - totalBytes;

	// hunt for the next sonar record
	while (bytesLeft > sizeof(RmLogItem))
	{
	  m_file.seek(totalBytes);

	  RmLogItem item;
	  quint64 readBytes = m_file.read((char*)&item, sizeof(RmLogItem));

	  if (readBytes != sizeof(RmLogItem))
		break;

	  // Record the position of the data record
	  quint64 filePos = totalBytes;

	  // Update total bytes read
	  totalBytes += readBytes;

	  if (item.itemHeader != RmLogger::s_itemHeader)
		break;

	  if (item.sizeHeader != sizeof(RmLogItem))
		break;

	  // If we are the type then add to the index list
	  if ((item.type == rt_oculusSonar) || (item.type == rt_apSonarHeader))
	  {
		*time = item.time;
		return true;
	  }

	  // Seek past in the payload
	  totalBytes += item.payloadSize;

	  bytesLeft = m_fileSize - totalBytes;
	}



	// ###########################

	// Record the position of the data record
	//quint64 filePos = m_totalBytes;

	// Update total bytes read
	//m_totalBytes += readBytes;

	/*
	if (item.itemHeader != RmLogger::s_itemHeader)
	  return ReadFailed("Not a valid item header");

	if (item.sizeHeader != sizeof(RmLogItem))
	  return ReadFailed("Item header version mismatch");


	*time = item.time;
*/
	return false;
}

// ----------------------------------------------------------------------------
// Read and broadcast an item from the current file if it is of the given type
bool RmPlayer::ReadNextItemOfType(quint16 type)
{
  if (!m_file.isOpen())
    return false;

  while (m_totalBytes < m_fileSize)
  {
    quint64 bytesLeft = m_fileSize - m_totalBytes;

    if (bytesLeft < sizeof(RmLogItem))
      return ReadFailed("Incomplete item header found");

    RmLogItem item;

    quint64 readBytes = m_file.read((char*)&item, sizeof(RmLogItem));

    if (readBytes != sizeof(RmLogItem))
      return ReadFailed("Cannot read the item header");

    // Record the position of the data record
    quint64 filePos = m_totalBytes;

    // Update total bytes read
    m_totalBytes += readBytes;

    if (item.itemHeader != RmLogger::s_itemHeader)
      return ReadFailed("Not a valid item header");

    if (item.sizeHeader != sizeof(RmLogItem))
      return ReadFailed("Item header version mismatch");

    // Make sure there is enough room in the buffer
    m_pPayloadBuffer = (quint8*) realloc (m_pPayloadBuffer, item.payloadSize);

    // Try to read in the payload
    readBytes = m_file.read((char*)m_pPayloadBuffer, item.payloadSize);

    if (readBytes != item.payloadSize)
      return ReadFailed("Unable to read item payload");

    // Update total bytes read
    m_totalBytes += item.payloadSize;

    if (item.type == type)
    {
      // Emit the new data packet for processing
      emit NewPayload(item.type, item.version, item.time, item.payloadSize, m_pPayloadBuffer);

      // Emit 2nd signal with file pos of the new data packet for processing
      emit NewPayloadPos(item.type, item.version, item.time, item.payloadSize, m_pPayloadBuffer, filePos);

      return true;
    }
  }

  return false;
}
