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
#include <utility>
#include <Windows.h>
#include "Data.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    namespace win
    {
      Data::Data(const Monitor& monitor, const unsigned long bufferLength)
        :
        _lpCompletionRoutine(nullptr), 
        _object(nullptr),
        _folder(nullptr),
        _hDirectory(nullptr),
        _buffer(nullptr),
        _bufferLength(bufferLength),
        _monitor(monitor)
      {
      }

      Data::~Data()
      {
        Close();

        // clear the folder.
        delete _folder;
        _folder = nullptr;
      }

      /**
       * \brief Prepare the buffer and structure for processing.
       * \param object the object we would like to pass to the `OVERLAPPED` structure.
       * \param bufferLength the lenght of the buffer.
       * \param lpCompletionRoutine the routine we will be calling when we get a valid notification
       */
      void Data::PrepareMonitor(void* object, const unsigned long bufferLength, const _COMPLETION_ROUTINE lpCompletionRoutine)
      {
        // make sure that the buffer is clear
        ClearBuffer();

        // the buffer
        _bufferLength = bufferLength;

        // the object
        _object = object;

        // save the completion source.
        _lpCompletionRoutine = lpCompletionRoutine;

        // then we can create it.
        _buffer = new unsigned char[_bufferLength];
        memset(_buffer, 0, sizeof(unsigned char)*_bufferLength);

        // save the overlapped opject.
        ClearOverlapped();
        _overlapped.hEvent = this;
      }

      /**
       * \brief Clear all the data
       */
      void Data::Close()
      {
        // close the folder
        CloseFolder();

        // the handle 
        ClearHandle();

        // the buffer.
        ClearBuffer();

        // clear the overlapped structure.
        ClearOverlapped();
      }

      /**
       * \brief if we opened the folder data, close it now.
       */
      void Data::CloseFolder() const
      {
        if( nullptr == _folder )
        {
          return;
        }
        _folder->Close();
      }


      /**
       * \brief Clear the handle
       */
      void Data::ClearHandle()
      {
        //  is it open?
        if (IsValidHandle())
        {
          try
          {
            CloseHandle(_hDirectory);
          }
          catch(...)
          {
            // We can ignore this... as per the doc:
            //   If the application is running under a debugger, the function will throw an exception if it receives either a handle value that is not valid or a pseudo-
            //   handle value. This can happen if you close a handle twice, or if you call CloseHandle on a handle returned by the FindFirstFile function instead of 
            //   calling the FindClose function.
          }
        }
        // the directory is closed.
        _hDirectory = nullptr;
      }

      /**
       * \brief clear the buffer data.
       */
      void Data::ClearBuffer()
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
      void Data::ClearOverlapped()
      {
        memset(&_overlapped, 0, sizeof(OVERLAPPED));
      }

      /**
       * \brief Check if the file is open properly
       * \return if the file has been open already.
       */
      bool Data::IsValidHandle() const
      {
        return _hDirectory != nullptr && _hDirectory != INVALID_HANDLE_VALUE;
      }

      /**
       * \brief get the directory handle, if we have one.
       * \return the handle
       */
      void* Data::DirectoryHandle() const
      {
        return _hDirectory;
      }

      /**
       * \brief get the current buffer
       * \return the void* buffer.
       */
      void* Data::Buffer() const
      {
        return _buffer;
      }

      /**
       * \brief get the buffer length
       * \return the buffer lenght
       */
      unsigned long Data::BufferLength() const
      {
        return _bufferLength;
      }

      /**
       * \brief Get pointer to our overlapped data.
       * \return pointer to the 'OVERLAPPED' struct.
       */
      LPOVERLAPPED Data::Overlapped()
      {
        return &_overlapped;
      }

      /**
       * \brief clone up to 'ulSize' bytes into a buffer.
       *        it is up to the caller to clear/delete the buffer.
       * \param ulSize the max numberof bytes we want to copy
       * \return the cloned data.
       */
      unsigned char* Data::Clone(const unsigned long ulSize) const
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

      bool Data::Open( )
      {
        // check if this was done alread
        // we cannot use IsOpen() as INVALID_HANDLE_VALUE would cause a return false.
        if (DirectoryHandle() != nullptr)
        {
          return DirectoryHandle() != INVALID_HANDLE_VALUE;
        }

        // how we want to open this directory.
        const auto shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
        const auto fileAttr = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED;

        _hDirectory = CreateFileW(
          _monitor.Path().c_str(),		// the path we are watching
          FILE_LIST_DIRECTORY,        // required for ReadDirectoryChangesW( ... )
          shareMode,
          nullptr,                    // security descriptor
          OPEN_EXISTING,              // how to create
          fileAttr,
          nullptr                     // file with attributes to copy
        );

        // check if it all worked.
        return IsValidHandle();
      }

      /**
       * \brief start monitoring a given folder.
       * \param object the object we will be passed by the overlapped param
       * \param notifyFilter the notification filter, (what we are watching the folder for)
       * \param recursive recursively check the given folder or not.
       * \param lpCompletionRoutine the completion routine we will call.
       * \return success or not
       */
      bool Data::Start(void* object, const unsigned long notifyFilter, const bool recursive, const _COMPLETION_ROUTINE lpCompletionRoutine)
      {
        // prepare all the values
        PrepareMonitor(object, _bufferLength, lpCompletionRoutine);

        // This call needs to be reissued after every APC.
        if( !::ReadDirectoryChangesW(
          DirectoryHandle(),
          Buffer(),
          BufferLength(),
          recursive,
          notifyFilter,
          nullptr,                // bytes returned, (not used here as we are async)
          Overlapped(),           // buffer with our information
          &FileIoCompletionRoutine
        ))
        {
          return false;
        }

        // If we are not monitoring folder changes
        // then we need to check our own folder withour recursion.
        // that way, if it is deleted, we will be able to release the handle(s).
        if ((notifyFilter & FILE_NOTIFY_CHANGE_DIR_NAME) != FILE_NOTIFY_CHANGE_DIR_NAME)
        {
          if( nullptr == _folder )
          {
            _folder = new Data(_monitor, _bufferLength);
          }
          if(_folder->_hDirectory == nullptr )
          {
            _folder->Open();
          }
          if (_folder->IsValidHandle() )
          {
            _folder->Start(nullptr, FILE_NOTIFY_CHANGE_DIR_NAME, false, lpCompletionRoutine);
          }
        }
        return true;
      }

      /***
       * \brief The async callback function for ReadDirectoryChangesW
       */
      void CALLBACK Data::FileIoCompletionRoutine(
        const unsigned long dwErrorCode,
        const unsigned long dwNumberOfBytesTransfered,
        _OVERLAPPED* lpOverlapped
      )
      {
        // get the object we are working with
        const auto obj = static_cast<Data*>(lpOverlapped->hEvent);

        switch (dwErrorCode)
        {
        case ERROR_SUCCESS:// all good, continue;
          break;

        case ERROR_OPERATION_ABORTED:
          if (obj != nullptr)
          {
            obj->_monitor.AddEventError(EventError::Aborted);
          }
          return;

        case ERROR_ACCESS_DENIED:
          obj->Close();
          if (obj != nullptr)
          {
            obj->_monitor.AddEventError(EventError::Access);
          }
          return;

        default:
          // https://msdn.microsoft.com/en-gb/574eccda-03eb-4e8a-9d74-cfaecc7312ce?f=255&MSPPError=-2147217396
          OVERLAPPED stOverlapped;
          DWORD dwBytesRead = 0;
          if (!GetOverlappedResult(obj->DirectoryHandle(),
              &stOverlapped,
              &dwBytesRead,
              FALSE))
          {
            const auto dwError = GetLastError();
            obj->_monitor.AddEventError(EventError::Overflow);
          }
          return;
        }

        // do we have an object?
        // should never happen ... but still.
        if (nullptr == obj)
        {
          return;
        }

        obj->_lpCompletionRoutine(dwNumberOfBytesTransfered, obj->_object, *obj);
      }
    }
  }
}