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
  namespace
  {
    /**
     * \brief predicate used while waiting for the operation-aborted message.
     */
    struct operation_aborted_predicate final
    {
      explicit operation_aborted_predicate(std::atomic<bool>& operationAborted) :
        _operationAborted(operationAborted)
      {
      }

      bool operator()() const
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
      }

    private:
      std::atomic<bool>& _operationAborted;
    };
  }

  Data::Data(
    const long long id,
    const wchar_t* path,
    const unsigned long notifyFilter,
    const bool recursive,
    const unsigned long bufferLength,
    threads::WorkerPool& workerPool
    )
    :
    _stopWorker( nullptr ),
    _workerPool( workerPool ),
    _invalidHandleWait(0),
    _notifyFilter(notifyFilter),
    _recursive(recursive),
    _operationAborted( false ),
    _hDirectory(nullptr),
    _buffer(nullptr),
    _bufferLength(bufferLength),
    _path( path ),
    _id( id )
  {
    // prepapre the buffer that will receive our data
    // create the buffer if needed.
    _buffer = new unsigned char[_bufferLength];
  }

  Data::~Data()
  {
    stop_and_wait();
  }

  /**
   * \brief prepare the various buffer for changes.
   */
  void Data::prepare_for_read()
  {
    MYODDWEB_PROFILE_FUNCTION();
    if( _operationAborted || _colectionState != CollectionState::Started )
    {
      return;
    }

    // restart the buffer.
    memset(_buffer, 0, sizeof(unsigned char)*_bufferLength);

    // reset our overlappped object
    clear_overlapped();

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
  bool Data::start()
  {
    // only start if we are unknown
    // otherwise just get out
    if(_colectionState != CollectionState::Unknown )
    {
      return false;
    }
    _colectionState = CollectionState::Started;

    // try and open the directory
    // if it is open already then nothing should happen here.
    if (!open_directory_handle())
    {
      // we could not access this
      Logger::log(_id, LogLevel::Warning, L"Unable to read directory: %s", _path.c_str());
      return false;
    }

    // reset the handle wait.
    _invalidHandleWait = 0;

    // start reading.
    listen();
    return true;
  }

  /// <summary>
  /// Stop monitoring data and wait for the work to complete.
  /// </summary>
  void Data::stop_and_wait()
  {
    MYODDWEB_LOCK(_stopWorkerLock);

    //  call the stop
    stop_in_lock();

    if (_stopWorker != nullptr)
    {
      _workerPool.wait_for(*_stopWorker, -1);

      // and then wait for it to complete
      const auto wr = _stopWorker->stop_and_wait(-1);
      if( wr != threads::WaitResult::complete )
      {
        Logger::log(LogLevel::Error, L"Unable to complete Data worker" );
      }

      // we are done with this worker
      _workerPool.stop_and_wait(*_stopWorker, -1 );

      // we are done with this worker
      delete _stopWorker;
    }

    // we are done already.
    _stopWorker = nullptr;
  }


  /**
   * \brief Clear all the data and close all connections handles.
   */
  void Data::stop()
  {
    MYODDWEB_LOCK(_stopWorkerLock);
    stop_in_lock();
  }

  /// <summary>
  /// Stop monitoring folder while we have the stop lock.
  /// </summary>
  void Data::stop_in_lock()
  {
    _colectionState = CollectionState::Stopped;
    try
    {
      // are we stopping already?
      // no point in doing this again.
      if( nullptr != _stopWorker )
      {
        return;
      }

      // make sure we do no start a new thread for no reason
      if( !is_valid_handle() )
      {
        return;
      }

      // start a worker to stop everything.
      // the stop flag is set so we should not be able to re-start anything
      // we can move on and let other threads do their things.
      _stopWorker = new threads::CallbackWorker( stop_worker_task(*this) );

      // add this to the pool
      _workerPool.add( *_stopWorker );
    }
    catch ( std::exception& e)
    {
      // log the error
      Logger::log(LogLevel::Error, L"Caught exception '%hs' in StopMonitoring!", e.what());
    }
  }

  /**
   * \brief Clear the handle
   * \return true if we confirmed that no I/O operation is still pending against
   *         our buffer/overlapped structure, (so it is now safe to free them).
   *         false if we could not get that confirmation, (timeout or error),
   *         in which case the caller must not free the buffer/overlapped as
   *         the kernel might still complete the read into them later.
   */
  bool Data::clear_handle()
  {
    // is it valid?
    if (!is_valid_handle())
    {
      // make sure that the handle is null
      // as it couls also be 0xffffff
      _hDirectory = nullptr;
      return true;
    }

    if( nullptr == _overlapped)
    {
      return true;
    }

    auto confirmedAborted = false;
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
        if (!_operationAborted && !Wait::spin_until(operation_aborted_predicate(_operationAborted), MYODDWEB_WAITFOR_OPERATION_ABORTED_COMPLETION))
        {
          Logger::log(_id, LogLevel::Warning, L"Timeout waiting operation aborted message!" );
        }

        // we can only be sure it is safe to free the buffer/overlapped
        // if we actually received confirmation that the operation was aborted.
        // if we timed out we do _not_ know if the kernel still holds a
        // reference to them, so the caller must not free them.
        confirmedAborted = _operationAborted;
      }
      else
      {
        // CancelIoEx could not find the handle/OVERLAPPED, which means
        // there was nothing pending for it to cancel in the first place.
        const auto dw = ::GetLastError();
        _operationAborted = true;
        confirmedAborted = true;
      }
      ::CloseHandle(_hDirectory);
    }
    catch (...)
    {
      // We can ignore this... as per the doc:
      //   If the application is running under a debugger, the function will throw an exception if it receives either a handle value that is not valid or a pseudo-
      //   handle value. This can happen if you close a handle twice, or if you call CloseHandle on a handle returned by the FindFirstFile function instead of
      //   calling the FindClose function.
      Logger::log( _id, LogLevel::Information, L"Ignore: Error waiting operation aborted message." );

      // we do _not_ know if the operation was really aborted, so we cannot
      // safely say that it is safe to free the buffer/overlapped structure.
      confirmedAborted = false;
    }

    // the directory is closed, (but the buffer/overlapped might still be in use).
    return confirmedAborted;
  }

  /**
   * \brief log that we intentionally leaked the buffer/overlapped because
   *        we could not confirm that the pending read was cancelled.
   */
  void Data::log_leaked_buffer_on_unconfirmed_stop() const
  {
    Logger::log(_id, LogLevel::Warning, L"Leaking watch buffer for '%ls': could not confirm that the pending read was cancelled in time.", _path.c_str());
  }

  /**
   * \brief clear the buffer data.
   */
  void Data::clear_buffer()
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
    catch (std::exception& e)
    {
      // the callback did something wrong!
      // log the error
      Logger::log(LogLevel::Error, L"Caught exception '%hs' in PublishStatistics, check the callback!", e.what());
      _buffer = nullptr;
    }
  }

  /// <summary>
  /// Clear all the data that is left in the vector
  /// </summary>
  void Data::clear_data()
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
  void Data::clear_overlapped()
  {
    delete _overlapped;
    _overlapped = nullptr;
  }

  /**
   * \brief Check if the file is open properly
   * \return if the file has been open already.
   */
  bool Data::is_valid_handle() const
  {
    return _hDirectory != nullptr && _hDirectory != INVALID_HANDLE_VALUE;
  }

  /**
   * \brief clone up to 'ulSize' bytes into a buffer.
   *        it is up to the caller to clear/delete the buffer.
   * \param ulSize the max numberof bytes we want to copy
   * \return the cloned data.
   */
  unsigned char* Data::clone(const unsigned long ulSize) const
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
    catch ( std::exception& e)
    {
      // the callback did something wrong!
      // log the error
      Logger::log(LogLevel::Error, L"Caught exception '%hs' in PublishStatistics, check the callback!", e.what());
      return nullptr;
    }
  }

  /**
   * \brief set the directory handle
   * \return if success or not.
   */
  bool Data::open_directory_handle( )
  {
    // check if this was done already
    if (is_valid_handle())
    {
      return true;
    }

    // how we want to open this directory.
    const auto shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    const auto fileAttr = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED;

    try
    {
      const auto handle = CreateFileW(
        _path.c_str(),            // the path we are watching
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
  bool Data::listen()
  {
    // if we are not started then we do not want to start
    if(_colectionState != CollectionState::Started )
    {
      return false;
    }

    MYODDWEB_PROFILE_FUNCTION();
    try
    {
      // prepare all the values
      prepare_for_read();

      // do the actual read.
      if (::ReadDirectoryChangesW(
        _hDirectory,
        _buffer,
        _bufferLength,
        _recursive ? 1 : 0,
        _notifyFilter,
        nullptr,                // bytes returned, (not used here as we are async)
        _overlapped,            // buffer with our information
        &file_io_completion_routine
      ) != 1)
      {
        // we could not create the monitoring
        stop();
        return false;
      }
      return true;
    }
    catch (std::exception& e)
    {
      // the callback did something wrong!
      // log the error
      Logger::log(LogLevel::Error, L"Caught exception '%hs' in PublishStatistics, check the callback!", e.what());
      return false;
    }
  }

  /***
   * \brief The async callback function for ReadDirectoryChangesW
   */
  void Data::file_io_completion_routine(
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
        Logger::log(LogLevel::Error, L"file_io_completion_routine received a null Data pointer.");
        return;
      }

      if (ERROR_SUCCESS != dwErrorCode)
      {
        // some other error
        data->process_error(dwErrorCode);
        return;
      }

      // success
      data->process_read(dwNumberOfBytesTransfered);
    }
    catch (std::exception& e)
    {
      // the callback did something wrong!
      // log the error
      Logger::log(LogLevel::Error, L"Caught exception '%hs' in PublishStatistics, check the callback!", e.what());
    }
  }

  /**
     * \brief process an error code.
     * \param errorCode the error received.
     */
  void Data::process_error( const unsigned long errorCode)
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

    case ERROR_NETNAME_DELETED:
      stop();
      Logger::log(LogLevel::Warning, L"Warning: The network connection to '%hs' has been deleted.", _path.c_str() );
      return;

    case ERROR_ACCESS_DENIED:
      stop();
      Logger::log(LogLevel::Warning, L"Warning: Acess to '%hs' is denied", _path.c_str() );
      return;

    default:
      // this is not one of the fatal errors, (network deleted / access denied),
      // that we handle above, so we must assume it is transient and re-issue
      // the read ourselves, otherwise this directory would silently stop
      // reporting any further changes with no way to recover.
      Logger::log( LogLevel::Warning, L"Warning: There was an error processing an API message %lu.", errorCode );
      listen();
      break;
    }
  }

  /**
   * \brief process a read received.
   * \param dwNumberOfBytesTransfered the number of bytes received.
   */
  void Data::process_read( const unsigned long dwNumberOfBytesTransfered )
  {
    if (dwNumberOfBytesTransfered == 0)
    {
      // Get the new read issued as fast as possible. The documentation
      // says that the original OVERLAPPED structure will not be used
      // again once the completion routine is called.
      listen();

      // we are done
      return;
    }

    // Can't use sizeof(FILE_NOTIFY_INFORMATION) because
    // the structure is padded to 16 bytes.
    _ASSERTE(dwNumberOfBytesTransfered >= offsetof(FILE_NOTIFY_INFORMATION, FileName) + sizeof(WCHAR));

    // clone the data now
    const auto cloned = clone(dwNumberOfBytesTransfered);

    // Get the new read issued as fast as possible. The documentation
    // says that the original OVERLAPPED structure will not be used
    // again once the completion routine is called.
    listen();

    // call the derived function to handle this.
    MYODDWEB_LOCK(_dataLock);
    _data.emplace_back( cloned );
  }

  std::vector<unsigned char*> Data::get()
  {
    MYODDWEB_LOCK(_dataLock);
    const auto cloned = _data;

    // clear that list
    // we do not want to use `shrink_to_fit` as the reserved value
    // will probably be reused.
    _data.clear();

    return cloned;
  }

  /**
   * \brief check that he current handle is still valie
   *        if not then we will close the connection.
   */
  void Data::check_still_valid()
  {
    if (is_valid_handle())
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

    // reset the start flag so we can restart
    _colectionState = CollectionState::Unknown;

    // try open again, if this does not work then it is fine
    // because we have reset the timer
    start();
  }
}
