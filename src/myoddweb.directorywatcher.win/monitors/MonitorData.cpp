//This file is part of Myoddweb.Directorywatcher.
//
//    Myoddweb.Directorywatcher is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    Myoddweb.Directorywatcher is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Myoddweb.Directorywatcher.  If not, see<https://www.gnu.org/licenses/gpl-3.0.en.html>.
#include <cstring>
#include <windows.h>
#include "MonitorData.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    MonitorData::MonitorData(const unsigned long bufferLength) :
      _hDirectory(nullptr),
      _buffer(nullptr),
      _bufferLength(bufferLength)
    {
      memset(&_overlapped, 0, sizeof(OVERLAPPED));
    }

    MonitorData::~MonitorData()
    {
      Clear();
    }

    /**
     * \brief Prepare the buffer and structure for processing.
     * \param object the object we would like to pass to the `OVERLAPPED` structure.
     */
    void MonitorData::Prepare(void* object)
    {
      // make sure that the buffer is clear
      ClearBuffer();

      // then we can create it.
      _buffer = new unsigned char[_bufferLength];
      memset(_buffer, 0, sizeof(unsigned char)*_bufferLength);

      // save the overlapped opject.
      ClearOverlapped();
      _overlapped.hEvent = object;
    }

    /**
     * \brief Clear all the data
     */
    void MonitorData::Clear()
    {
      // the handle 
      ClearHandle();

      // the buffer.
      ClearBuffer();

      // clear the overlapped structure.
      ClearOverlapped();
    }

    /**
     * \brief Clear the handle
     */
    void MonitorData::ClearHandle()
    {
      //  is it open?
      if (IsValidHandle())
      {
        CloseHandle(_hDirectory);
      }
      // the directory is closed.
      _hDirectory = nullptr;
    }

    /**
     * \brief clear the buffer data.
     */
    void MonitorData::ClearBuffer()
    {
      if (_buffer == nullptr)
      {
        return;
      }
      delete[] _buffer;
      _buffer = nullptr;
    }

    /**
     * \brief clear the overlapped structure.
     */
    void MonitorData::ClearOverlapped()
    {
      memset(&_overlapped, 0, sizeof(OVERLAPPED));
    }

    /**
     * \brief Check if the file is open properly
     * \return if the file has been open already.
     */
    bool MonitorData::IsValidHandle() const
    {
      return _hDirectory != nullptr && _hDirectory != INVALID_HANDLE_VALUE;
    }

    /**
     * \brief get the directory handle, if we have one.
     * \return the handle
     */
    void* MonitorData::DirectoryHandle() const
    {
      return _hDirectory;
    }

    /**
     * \brief set the directory handle
     * \param handle the directory handle we want to save
     */
    void MonitorData::DirectoryHandle(void* handle)
    {
      _hDirectory = handle;
    }

    /**
     * \brief get the current buffer
     * \return the void* buffer.
     */
    void* MonitorData::Buffer() const
    {
      return _buffer;
    }

    /**
     * \brief get the buffer length
     * \return the buffer lenght
     */
    unsigned long MonitorData::BufferLength() const
    {
      return _bufferLength;
    }

    /**
     * \brief Get pointer to our overlapped data.
     * \return pointer to the 'OVERLAPPED' struct.
     */
    LPOVERLAPPED MonitorData::Overlapped()
    {
      return &_overlapped;
    }

    /**
     * \brief clone up to 'ulSize' bytes into a buffer.
     *        it is up to the caller to clear/delete the buffer.
     * \param ulSize the max numberof bytes we want to copy
     * \return the cloned data.
     */
    unsigned char* MonitorData::Clone(const unsigned long ulSize) const
    {
      // if the size if more than we can offer we need to prevent an overflow.
      if (ulSize > _bufferLength)
      {
        return nullptr;
      }

      // create the clone
      const auto pBuffer = new unsigned char[ulSize];

      // copy it.
      memcpy(pBuffer, _buffer, ulSize);

      // return it.
      return pBuffer;
    }
  }
}
