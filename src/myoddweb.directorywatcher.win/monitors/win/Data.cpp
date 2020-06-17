// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#include <cstring>
#include <utility>
#include "Data.h"
#include "../../utils/Instrumentor.h"
#include "../../utils/Lock.h"
#include "../../utils/Logger.h"
#include "../../utils/LogLevel.h"
#include "../../utils/Wait.h"
#include "../Base.h"

namespace myoddweb:: directorywatcher:: win
{
  Data::Data(
    const long long id,
    const wchar_t* path,
    const unsigned long notifyFilter,
    const bool recursive,
    const unsigned long bufferLength
    )
    :
    _invalidHandleWait(0),
    _notifyFilter(notifyFilter),
    _recursive(recursive),
    _operationAborted( false ),
    _hDirectory(nullptr),
    _buffer(nullptr),
    _bufferLength(bufferLength),
    _path( path ),
    _id( id ),
    _overlapped(nullptr)
  {
    // prepapre the buffer that will receive our data
    // create the buffer if needed.
    _buffer = new unsigned char[_bufferLength];
  }

  Data::~Data()
  {
    Stop();
  }

  /**
   * \brief prepare the various buffer for changes.
   */
  void Data::PrepareForRead()
  {
    MYODDWEB_PROFILE_FUNCTION();
    if( _operationAborted || _stop )
    {
      return;
    }
    
    // restart the buffer.
    memset(_buffer, 0, sizeof(unsigned char)*_bufferLength);

    // reset our overlappped object
    ClearOverlapped();

    // create a new one with our own data
    _overlapped = new OVERLAPPED_DATA();
    memset(_overlapped, 0, sizeof(OVERLAPPED_DATA));

    // save the handle as well as this class so we can access it later.
    _overlapped->hEvent = _hDirectory;
    _overlapped->pdata = this;

    // assume that we are not aborted
    _operationAborted = false;
  }

  /**
   * \brief start monitoring the given folder.
   * \return if we managed to start the monitoring or not.
   */
  bool Data::Start()
  {
    _stop = false;

    // try and open the directory
    // if it is open already then nothing should happen here.
    if (!OpenDirectoryHandle())
    {
      // we could not access this
      Logger::Log(_id, LogLevel::Warning, L"Unable to read directory: %s", _path.c_str());
      return false;
    }

    // reset the handle wait.
    _invalidHandleWait = 0;

    // start reading.
    Listen();
    return true;
  }

  /**
   * \brief Clear all the data and close all connections handles.
   */
  void Data::Stop()
  {
    _stop = true;
    try
    {
      // close the handle 
      ClearHandle();

      // the buffer.
      ClearBuffer();

      // clear the overlapped structure.
      ClearOverlapped();

      // clear the buffer of data that might be left
      ClearData();
    }
    catch (const std::exception& e)
    {
      // log the error
      Logger::Log(LogLevel::Error, L"Caught exception '%hs' in StopMonitoring!", e.what());
    }
  }

  /**
   * \brief Clear the handle
   */
  void Data::ClearHandle()
  {
    // is it valid?
    if (!IsValidHandle())
    {
      // make sure that the handle is null
      // as it couls also be 0xffffff
      _hDirectory = nullptr;
      return;
    }

    if( nullptr == _overlapped)
    {
      return;
    }

    try
    {
      // tell all the pending reads that we are ready
      // to stop handling messages now.
      _operationAborted = false;

      // flag that we want to cancel the operation
      // if `CancelIoEx` returns zero then it means that either the handle
      // and/or the OVERLAPPED pointer could not be found.
      // \see https://docs.microsoft.com/en-us/windows/win32/fileio/cancelioex-func

      // then wait again for abort, (if needed)
      // in case any other messages are unprocessed.
      unsigned long numberOfBytes = 0;
      if (::CancelIoEx(_hDirectory, _overlapped) != 0 )
      {
        for (;;)
        {
          const auto status = WaitForSingleObjectEx(_hDirectory, 500, true );
          if( status == WAIT_IO_COMPLETION)
          {
            break;
          }
          if (status == WAIT_OBJECT_0)
          {
            break;
          }
        }

        // then wait a little for the operation to be cancelled.
        if (!_operationAborted && !Wait::SpinUntil([&]
          {
            if( _operationAborted != true )
            {
              // wait a little to ensure that the aborted message is given.
              // we will return as soon as the message is recived.
              // if we do not wait for the abort message, we might get other
              // messages out of sequence.
              MYODDWEB_YIELD();
            }
            return _operationAborted == true;
          }, MYODDWEB_WAITFOR_OPERATION_ABORTED_COMPLETION))
        {
          Logger::Log(_id, LogLevel::Warning, L"Timeout waiting operation aborted message!" );
        }
      }
      else
      {
        const auto dw = ::GetLastError();
        _operationAborted = true;
      }
      ::CloseHandle(_hDirectory);
    }
    catch (...)
    {
      // We can ignore this... as per the doc:
      //   If the application is running under a debugger, the function will throw an exception if it receives either a handle value that is not valid or a pseudo-
      //   handle value. This can happen if you close a handle twice, or if you call CloseHandle on a handle returned by the FindFirstFile function instead of 
      //   calling the FindClose function.
      Logger::Log( _id, LogLevel::Information, L"Ignore: Error waiting operation aborted message." );
      _operationAborted = true;
    }

    // the directory is closed.
  }

  /**
   * \brief clear the buffer data.
   */
  void Data::ClearBuffer()
  {
    try
    {
      if (_buffer == nullptr)
      {
        return;
      }

      delete[] _buffer;
      _buffer = nullptr;
    }
    catch (const std::exception& e)
    {
      // the callback did something wrong!
      // log the error
      Logger::Log(LogLevel::Error, L"Caught exception '%hs' in PublishStatistics, check the callback!", e.what());
      _buffer = nullptr;
    }
  }

  /// <summary>
  /// Clear all the data that is left in the vector
  /// </summary>
  void Data::ClearData()
  {
    MYODDWEB_LOCK(_dataLock);
    for( const auto &raw : _data )
    {
      delete[] raw;
    }
    _data.clear();
  }

  /**
   * \brief clear the overlapped structure.
   */
  void Data::ClearOverlapped()
  {
    delete _overlapped;
    _overlapped = nullptr;
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
   * \brief clone up to 'ulSize' bytes into a buffer.
   *        it is up to the caller to clear/delete the buffer.
   * \param ulSize the max numberof bytes we want to copy
   * \return the cloned data.
   */
  unsigned char* Data::Clone(const unsigned long ulSize) const
  {
    try
    {
      // if the size if more than we can offer we need to prevent an overflow.
      if (ulSize > _bufferLength)
      {
        return nullptr;
      }

      // create the clone
      const auto pBuffer = new unsigned char[ulSize];
      if (_buffer == nullptr)
      {
        return nullptr;
      }

      // copy it.
      memcpy(pBuffer, _buffer, ulSize);

      // return it.
      return pBuffer;
    }
    catch (const std::exception& e)
    {
      // the callback did something wrong!
      // log the error
      Logger::Log(LogLevel::Error, L"Caught exception '%hs' in PublishStatistics, check the callback!", e.what());
      return nullptr;
    }
  }

  /**
   * \brief set the directory handle
   * \return if success or not.
   */
  bool Data::OpenDirectoryHandle( )
  {
    // check if this was done already
    if (IsValidHandle())
    {
      return true;
    }

    // how we want to open this directory.
    const auto shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    const auto fileAttr = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED;

    try
    {
      const auto handle = CreateFileW(
        _path.c_str(),  		        // the path we are watching
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

    // check if it all worked.
    return true;
  }

  /**
   * \brief start waiting for notification
   */
  bool Data::Listen()
  {
    if(_stop)
    {
      return false;
    }
    MYODDWEB_PROFILE_FUNCTION();
    try
    {
      // prepare all the values
      PrepareForRead();

      // do the actual read.
      if (::ReadDirectoryChangesW(
        _hDirectory,
        _buffer,
        _bufferLength,
        _recursive ? 1 : 0,
        _notifyFilter,
        nullptr,                // bytes returned, (not used here as we are async)
        _overlapped,            // buffer with our information
        &FileIoCompletionRoutine
      ) != 1)
      {
        // we could not create the monitoring
        Stop();
        return false;
      }
      return true;
    }
    catch (const std::exception& e)
    {
      // the callback did something wrong!
      // log the error
      Logger::Log(LogLevel::Error, L"Caught exception '%hs' in PublishStatistics, check the callback!", e.what());
      return false;
    }
  }

  /***
   * \brief The async callback function for ReadDirectoryChangesW
   */
  void Data::FileIoCompletionRoutine(
    const unsigned long dwErrorCode,
    const unsigned long dwNumberOfBytesTransfered,
    _OVERLAPPED* lpOverlapped
  )
  {
    try
    {
      // get the object we are working with
      const auto lpOverlappedData = reinterpret_cast<_OVERLAPPED_DATA*>(lpOverlapped);
      auto data = lpOverlappedData == nullptr ? nullptr : lpOverlappedData->pdata;

      // do we have an object?
      // should never happen ... but still.
      if (nullptr == data)
      {
        data->ProcessError(dwErrorCode);
        return;
      }

      if (ERROR_SUCCESS != dwErrorCode)
      {
        // some other error
        data->ProcessError(dwErrorCode);
        return;
      }

      // success
      data->ProcessRead(dwNumberOfBytesTransfered);
    }
    catch (const std::exception& e)
    {
      // the callback did something wrong!
      // log the error
      Logger::Log(LogLevel::Error, L"Caught exception '%hs' in PublishStatistics, check the callback!", e.what());
    }
  }

  /**
     * \brief process an error code.
     * \param errorCode the error received.
     */
  void Data::ProcessError( const unsigned long errorCode)
  {
    switch (errorCode)
    {
    case ERROR_SUCCESS:// all good, continue;
      break;

    case ERROR_OPERATION_ABORTED:
      // set the flag _after_ we posted the message above
      // as what happens after this is undefined.
      _operationAborted = true;
      break;

    case ERROR_ACCESS_DENIED:
      Stop();
      return;

    default:
      //  we cannot use _monitor anymore
      Logger::Log(0, LogLevel::Warning, L"Warning: There was an error processing an API message %lu.", errorCode );
      break;
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
      // Get the new read issued as fast as possible. The documentation
      // says that the original OVERLAPPED structure will not be used
      // again once the completion routine is called.
      Listen();

      // we are done
      return;
    }

    // Can't use sizeof(FILE_NOTIFY_INFORMATION) because
    // the structure is padded to 16 bytes.
    _ASSERTE(dwNumberOfBytesTransfered >= offsetof(FILE_NOTIFY_INFORMATION, FileName) + sizeof(WCHAR));

    // clone the data now
    const auto clone = Clone(dwNumberOfBytesTransfered);

    // Get the new read issued as fast as possible. The documentation
    // says that the original OVERLAPPED structure will not be used
    // again once the completion routine is called.
    Listen();

    // call the derived function to handle this.
    MYODDWEB_LOCK(_dataLock);
    _data.emplace_back( clone );
  }

  std::vector<unsigned char*> Data::Get()
  {
    MYODDWEB_LOCK(_dataLock);
    const auto clone = _data;

    // clear that list
    // we do not want to use `shrink_to_fit` as the reserved value
    // will probably be reused.
    _data.clear();

    return clone;
  }

  /**
   * \brief check that he current handle is still valie
   *        if not then we will close the connection.
   */
  void Data::CheckStillValid()
  {
    if (IsValidHandle())
    {
      // The handle is good, so we can reset the value
      _invalidHandleWait = 0;
      return;
    }

    // wait a little bit longer.
    _invalidHandleWait += MYODDWEB_MIN_THREAD_SLEEP;
    if (_invalidHandleWait < MYODDWEB_INVALID_HANDLE_SLEEP)
    {
      // we need to wait a little longer before we re-open
      return;
    }

    // we already know that the handle is not valid
    _hDirectory = nullptr;

    // we will reopen, so reset the wait time.
    _invalidHandleWait = 0;

    // try open again, if this does not work then it is fine
    // because we have reset the timer
    Start();
  }
}
