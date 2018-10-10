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
#pragma once
#include <windows.h>

namespace myoddweb
{
  namespace directorywatcher
  {
    class MonitorData
    {
    public:
      explicit MonitorData(unsigned long bufferLength);
      ~MonitorData();

      void Prepare(void* object);

      /**
       * \brief Clear all the data
       */
      void Clear();

      /**
       * Check if the handle is valid
       */
      bool IsValidHandle() const;

      /**
       * \brief get the directory handle, if we have one.
       * \return the handle
       */
      void* DirectoryHandle() const;

      /**
       * \brief set the directory handle
       * \param handle the directory handle we want to save
       */
      void DirectoryHandle(void* handle);

      /**
       * \brief clone up to 'ulSize' bytes into a buffer.
       *        it is up to the caller to clear/delete the buffer.
       * \param ulSize the max numberof bytes we want to copy
       * \return the cloned data.
       */
      unsigned char* Clone(unsigned long ulSize) const;

      /**
       * \brief get the current buffer
       * \return the void* buffer.
       */
      void* Buffer() const;

      /**
       * \brief get the buffer length
       * \return the buffer lenght
       */
      unsigned long BufferLength() const;

      /**
       * \brief Get pointer to our overlapped data.
       * \return pointer to the 'OVERLAPPED' struct.
       */
      LPOVERLAPPED Overlapped();

    private:
      void* _hDirectory;
      unsigned char* _buffer;
      const unsigned long _bufferLength;

      /**
       * \brief the overlapped
       */
      OVERLAPPED	_overlapped{};

      /**
       * \brief Clear the handle
       */
      void ClearHandle();

      /**
       * \brief clear the buffer data.
       */
      void ClearBuffer();

      /**
       * \brief clear the overlapped structure.
       */
      void ClearOverlapped();
    };
  }
}
