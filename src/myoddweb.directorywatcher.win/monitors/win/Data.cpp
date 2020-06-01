// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#include <cstring>
#include <utility>
#include "Data.h"
#include "../../utils/Instrumentor.h"
#include "../../utils/Wait.h"
#include "../Base.h"

namespace myoddweb:: directorywatcher:: win
{
  Data::Data(
    const Monitor& monitor, 
    const unsigned long notifyFilter,
    const bool recursive,
    DataCallbackFunction& dataCallbackFunction,
    const unsigned long bufferLength
    )
    :
    _invalidHandleWait(0),
    _notifyFilter(notifyFilter),
    _recursive(recursive),
    _operationAborted( false ),
    _dataCallbackFunction( dataCallbackFunction ),
    _hDirectory(nullptr),
    _buffer(nullptr),
    _bufferLength(bufferLength),
    _monitor(monitor)
  {
    // prepapre the buffer that will receive our data
    // create the buffer if needed.
    _buffer = new unsigned char[_bufferLength];
  }

  Data::~Data()
  {
    StopMonitoring();
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

    // assume that we are not aborted
    _operationAborted = false;
  }

  /**
   * \brief Clear all the data and close all connections handles.
   */
  void Data::StopMonitoring()
  {
    try
    {
      // close the handle 
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
  }

  /**
   * \brief Clear the handle
   */
  void Data::ClearHandle()
  {
    //  is it open?
    if (!IsValidHandle())
    {
      // make sure that the handle is null
      // as it couls also be 0xffffff
      _hDirectory = nullptr;
      return;
    }

    // get the lock and check again
    if (!IsValidHandle())
    {
      // make sure that the handle is null
      // as it couls also be 0xffffff
      _hDirectory = nullptr;
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

      const auto cancel = ::CancelIoEx(_hDirectory, Overlapped());
      if (0 != cancel)
      {
        // then wait a little for the operation to be cancelled.
        for (;;)
        {
          if (!Wait::SpinUntil([&]
            {
              // wait a little to ensure that the aborted message is given.
              // we will return as soon as the message is recived.
              // if we do not wait for the abort message, we might get other
              // messages out of sequence.
              MYODDWEB_YIELD();
              return _operationAborted == true;
            }, MYODDWEB_WAITFOR_OPERATION_ABORTED_COMPLETION))
          {
            MYODDWEB_OUT("Timeout waiting operation aborted message!\n");
            continue;
          }
          break;
        }
      }
      ::CloseHandle(_hDirectory);
    }
    catch (...)
    {
      // We can ignore this... as per the doc:
      //   If the application is running under a debugger, the function will throw an exception if it receives either a handle value that is not valid or a pseudo-
      //   handle value. This can happen if you close a handle twice, or if you call CloseHandle on a handle returned by the FindFirstFile function instead of 
      //   calling the FindClose function.
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
  unsigned char* Data::Clone(const unsigned long ulSize) const
  {
    // if the size if more than we can offer we need to prevent an overflow.
    if (ulSize > _bufferLength)
    {
      return nullptr;
    }

    // create the clone
    const auto pBuffer = new unsigned char[ulSize];
    if (_buffer == nullptr )
    {
      return nullptr;
    }

    // copy it.
    memcpy(pBuffer, _buffer, ulSize);

    // return it.
    return pBuffer;
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

    // do the actual read.
    if( ::ReadDirectoryChangesW(
      _hDirectory,
      Buffer(),
      BufferLength(),
      _recursive,
      _notifyFilter,
      nullptr,                // bytes returned, (not used here as we are async)
      Overlapped(),           // buffer with our information
      &FileIoCompletionRoutine
    ) != 1 )
    {
      // we could not create the monitoring
      _monitor.AddEventError(EventError::Access);
      StopMonitoring();
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
  void Data::ProcessError( const unsigned long errorCode)
  {
    switch (errorCode)
    {
    case ERROR_SUCCESS:// all good, continue;
      break;

    case ERROR_OPERATION_ABORTED:
      // we are stopping, so we can indicate that we 
      // are now completely ready to stop.
      _monitor.AddEventError(EventError::Aborted);

      // set the flag _after_ we posted the message above
      // as what happens after this is undefined.
      _operationAborted = true;
      break;

    case ERROR_ACCESS_DENIED:
      StopMonitoring();
      _monitor.AddEventError(EventError::Access);
      return;

    default:
      const auto dwError = GetLastError();
      _monitor.AddEventError(EventError::Overflow);
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
      BeginRead();

      // If the buffer overflows, the entire contents of the buffer are discarded, 
      // the lpBytesReturned parameter contains zero, and the ReadDirectoryChangesW function 
      // fails with the error code ERROR_NOTIFY_ENUM_DIR.
      _dataCallbackFunction(nullptr);

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
    BeginRead();

    // call the derived function to handle this.
    _dataCallbackFunction( clone );
  }

  /**
   * \brief start monitoring the given folder.
   * \return if we managed to start the monitoring or not.
   */
  bool Data::StartMonitoring()
  {
    // try and open the directory
    // if it is open already then nothing should happen here.
    if (!OpenDirectoryHandle())
    {
      // we could not access this
      _monitor.AddEventError(EventError::Access);
      return false;
    }

    // reset the handle wait.
    _invalidHandleWait = 0;

    // start reading.
    BeginRead();
    return true;
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
    StartMonitoring();
  }
}
