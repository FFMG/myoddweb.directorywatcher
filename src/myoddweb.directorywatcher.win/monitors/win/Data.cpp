// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#include <cstring>
#include <utility>
#include <Windows.h>
#include "Data.h"
#include "../../utils/Lock.h"
#include "../../utils/Instrumentor.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    namespace win
    {
      Data::Data(
        const Monitor& monitor, 
        const unsigned long notifyFilter,
        const bool recursive,
        DataCallbackFunction& dataCallbackFunction,
        const unsigned long bufferLength
        )
        :
        _recursive( recursive ),
        _notifyFilter(notifyFilter),
        _dataCallbackFunction( dataCallbackFunction ),
        _bufferLength(bufferLength),
        _hDirectory(nullptr),
        _buffer(nullptr),
        _monitor(monitor),
        _state( State::Unknown )
      {
        // prepapre the buffer that will receive our data
        // create the buffer if needed.
        _buffer = new unsigned char[_bufferLength];
      }

      Data::~Data()
      {
        Close();
      }

      /**
       * \brief prepare the various buffer for changes.
       */
      void Data::PrepareForRead()
      {
        MYODDWEB_PROFILE_FUNCTION();
        
        // restart the buffer.
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
        // no need for double lock
        auto guard = Lock(_lock);
        if (_state == State::Stopped)
        {
          return;
        }

//MY_TRACE("Closing %p\n", this );

        try
        {
          // the handle 
          ClearHandle();

          // the buffer.
          ClearBuffer();

          // clear the overlapped structure.
          ClearOverlapped();
        }
        catch (...)
        {
          // @todo log this
        }

        // we are now stopped.
        _state = State::Stopped;
      }

      /**
       * \brief Clear the handle
       */
      void Data::ClearHandle()
      {
        //  is it open?
        if (IsValidHandle())
        {
          auto guard = Lock(_lock);
          if (IsValidHandle())
          {
            try
            {
              ::CancelIo(_hDirectory);           
              ::CloseHandle(_hDirectory);
            }
            catch (...)
            {
              // We can ignore this... as per the doc:
              //   If the application is running under a debugger, the function will throw an exception if it receives either a handle value that is not valid or a pseudo-
              //   handle value. This can happen if you close a handle twice, or if you call CloseHandle on a handle returned by the FindFirstFile function instead of 
              //   calling the FindClose function.
            }
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
        auto guard = Lock(_lock);
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
        auto guard = Lock(_lock);
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
       * \return the buffer length
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
      unsigned char* Data::Clone(const unsigned long ulSize)
      {
        // if the size if more than we can offer we need to prevent an overflow.
        if (ulSize > _bufferLength)
        {
          return nullptr;
        }

        // create the clone
        const auto pBuffer = new unsigned char[ulSize];

        // use the lock to prevent buffer from going away 
        // while we are busy working here.
        auto guard = Lock(_lock);
        if (_buffer == nullptr || _state == State::Stopped )
        {
          return nullptr;
        }

        // copy it.
        memcpy(pBuffer, _buffer, ulSize);

        // return it.
        return pBuffer;
      }

      /**
       * \brief if there was a problem, try re-open the file.
       * \return if success or not.
       */
      bool Data::TryReopen()
      {
        try
        {
          // close if needed
          if (IsValidHandle())
          {
            Close();
          }

          // make sure that the handle is ready for reopen
          _hDirectory = nullptr;

          // or get out.
          return Open();
        }
        catch( ... )
        {
          // this did not work.
          return false;
        }
      }

      /**
       * \brief set the directory handle
       * \return if success or not.
       */
      bool Data::Open( )
      {
        // no need to worry about double lock
        auto guard = Lock(_lock);

        // check if this was done already
        // we cannot use IsOpen() as INVALID_HANDLE_VALUE would cause a return false.
        if (DirectoryHandle() != nullptr)
        {
          return DirectoryHandle() != INVALID_HANDLE_VALUE;
        }

        // are we started?
        if (_state == State::Started)
        {
          // we arelady started
          return true;
        }

        // how we want to open this directory.
        const auto shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
        const auto fileAttr = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED;

        try
        {
          const auto handle = CreateFileW(
            _monitor.Path(),		        // the path we are watching
            FILE_LIST_DIRECTORY,        // required for ReadDirectoryChangesW( ... )
            shareMode,
            nullptr,                    // security descriptor
            OPEN_EXISTING,              // how to create
            fileAttr,
            nullptr                     // file with attributes to copy
          );

          if (handle == INVALID_HANDLE_VALUE)
          {
            return false;
          }

          // set the handle.
          _hDirectory = handle;
        }
        catch (...)
        {
          _hDirectory = INVALID_HANDLE_VALUE;
          return false;
        }

        // we are now open
        _state = State::Started;

        // check if it all worked.
        return true;
      }

      /**
       * \brief start waiting for notification
       */
      bool Data::BeginRead()
      {
        MYODDWEB_PROFILE_FUNCTION();

        // prepare all the values
        PrepareForRead();

        return
        ::ReadDirectoryChangesW(
          DirectoryHandle(),
          Buffer(),
          BufferLength(),
          _recursive,
          _notifyFilter,
          nullptr,                // bytes returned, (not used here as we are async)
          Overlapped(),           // buffer with our information
          &FileIoCompletionRoutine
        ) == 1;
      }

      /**
       * \brief start monitoring a given folder.
       * \param notifyFilter the notification filter, (what we are watching the folder for)
       * \param recursive recursively check the given folder or not.
       * \param dataCallbackFunction the completion routine we will call.
       * \return success or not
       */
      bool Data::Start()
      {        
        if (_state == State::Stopped)
        {
          // we stopped.
          return false;
        }
        auto guard = Lock(_lock);
        if (_state == State::Stopped)
        {
          return false;
        }

        // This call needs to be reissued after every APC.
//MY_TRACE( "Waiting to read! %p\n", this );

        if( !BeginRead() )
        {
          return false;
        }

        return true;
      }

      /***
       * \brief The async callback function for ReadDirectoryChangesW
       */
      void __stdcall Data::FileIoCompletionRoutine(
        const unsigned long dwErrorCode,
        const unsigned long dwNumberOfBytesTransfered,
        _OVERLAPPED* lpOverlapped
      )
      {
        // get the object we are working with
        const auto data = static_cast<Data*>(lpOverlapped->hEvent);

//MY_TRACE("Done reading! %p\n", data);

        // do we have an object?
        // should never happen ... but still.
        if (nullptr == data)
        {
          return;
        }

        switch (dwErrorCode)
        {
        case ERROR_SUCCESS:// all good, continue;
          data->ProcessRead(dwNumberOfBytesTransfered);
          break;

        default:
          data->ProcessError(dwErrorCode);
          break;
        }
      }

      /**
         * \brief process an error code.
         * \param errorCode the error received.
         */
      void Data::ProcessError(unsigned long errorCode)
      {
        switch (errorCode)
        {
        case ERROR_SUCCESS:// all good, continue;
          break;

        case ERROR_OPERATION_ABORTED:
          _monitor.AddEventError(EventError::Aborted);
          return;

        case ERROR_ACCESS_DENIED:
          Close();
          _monitor.AddEventError(EventError::Access);
          return;

        default:
          const auto dwError = GetLastError();
          _monitor.AddEventError(EventError::Overflow);
          return;
        }
      }

      /**
       * \brief process a read received.
       * \param dwNumberOfBytesTransfered the number of bytes received.
       */
      void Data::ProcessRead( const unsigned long dwNumberOfBytesTransfered )
      {
        if (dwNumberOfBytesTransfered == 0)
        {
          _dataCallbackFunction(nullptr);
          return;
        }

        // Can't use sizeof(FILE_NOTIFY_INFORMATION) because
        // the structure is padded to 16 bytes.
        _ASSERTE(dwNumberOfBytesTransfered >= offsetof(FILE_NOTIFY_INFORMATION, FileName) + sizeof(WCHAR));

        // call the derived function to handle this.
        _dataCallbackFunction( Clone(dwNumberOfBytesTransfered) );
      }
    }
  }
}
