// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include <Windows.h>
#include "../Monitor.h"
#include "../../utils/Threads/CallbackWorker.h"

namespace myoddweb:: directorywatcher:: win
{
  class Data final
  {
    typedef struct _OVERLAPPED_DATA : _OVERLAPPED {
      Data* pdata;
    } OVERLAPPED_DATA, * LPOVERLAPPED_DATA;
  public:
    explicit Data(
      long long id,
      const wchar_t* path,
      unsigned long notifyFilter,
      bool recursive,
      unsigned long bufferLength,
      threads::WorkerPool& workerPool
    );
    ~Data();

    /**
     * \brief Prevent copy construction
     */
    Data() = delete;
    Data(const Data&) = delete;
    Data(Data&&) = delete;
    Data& operator=(const Data&) = delete;
    Data& operator=(Data&& other) = delete;

    /**
     * \brief start monitoring the given folder.
     * \return if we managed to start the monitoring or not.
     */
    bool start();

    /**
     * \brief Clear all the data
     */
    void stop();

    /// <summary>
    /// Get the buffer data
    /// </summary>
    /// <returns></returns>
    std::vector<unsigned char*> get();

    /**
     * \brief check that he current handle is still valie
     *        if not then we will close the connection.
     */
    void check_still_valid();
  private:
    /// <summary>
    /// The worker we will be using to stop collecting data
    /// </summary>
    threads::CallbackWorker* _stopWorker;

    /// <summary>
    /// Make sure we only stop the worker once.
    /// </summary>
    MYODDWEB_MUTEX _stopWorkerLock;

    /// <summary>
    /// The lock we are using to clear/update the buffer
    /// </summary>
    MYODDWEB_MUTEX _dataLock;

    /// <summary>
    /// The buffer of data
    /// </summary>
    std::vector<unsigned char*> _data;

    threads::WorkerPool& _workerPool;

    /// <summary>
    /// Task used to finish clearing up the handle/buffer/data once stop is requested.
    /// </summary>
    struct stop_worker_task final
    {
    public:
      explicit stop_worker_task(Data& data) :
        _data(data)
      {
      }

      void operator()() const
      {
        // close the handle
        _data.clear_handle();

        // the buffer.
        _data.clear_buffer();

        // clear the overlapped structure.
        _data.clear_overlapped();

        // clear the buffer of data that might be left
        _data.clear_data();
      }

    private:
      Data& _data;
    };

    /// <summary>
    /// Stop monitoring data and wait for the work to complete.
    /// </summary>
    void stop_and_wait();

    /// <summary>
    /// Stop monitoring folder while we have the stop lock.
    /// </summary>
    void stop_in_lock();

    /**
     * \brief Check if the handle is valid
     */
    [[nodiscard]]
    bool is_valid_handle() const;

    /**
     * \brief set the directory handle
     * \return if success or not.
     */
    bool open_directory_handle();

    /**
     * \brief start monitoring a given folder.
     * \return success or not
     */
    bool listen();

    /**
     * \brief prepare the various buffer for changes.
     */
    void prepare_for_read();

    /**
     * \brief process a read received.
     * \param dwNumberOfBytesTransfered the number of bytes received.
     */
    void process_read(unsigned long dwNumberOfBytesTransfered );

    /**
     * \brief process an error code.
     * \param errorCode the error received.
     */
    void process_error(unsigned long errorCode);

    /**
     * \brief clone up to 'ulSize' bytes into a buffer.
     *        it is up to the caller to clear/delete the buffer.
     * \param ulSize the max number of bytes we want to copy
     * \return the cloned data.
     */
    [[nodiscard]]
    unsigned char* clone(unsigned long ulSize) const;

    /// <summary>
    /// The function that will be called when a file event is detected.
    /// </summary>
    /// <param name="dwErrorCode"></param>
    /// <param name="dwNumberOfBytesTransfered"></param>
    /// <param name="lpOverlapped"></param>
    /// <returns></returns>
    static void __stdcall file_io_completion_routine(
      unsigned long dwErrorCode,         // completion code
      unsigned long dwNumberOfBytesTransfered, // number of bytes transferred
      _OVERLAPPED* lpOverlapped                 // I/O information buffer
    );

    #pragma region Variables

    /**
     * \brief the number of times we had an invalid handle.
     *        after a certain count we will close this.
     */
    int _invalidHandleWait;

    /**
     * \brief what we wish to be notified about
     * \see https://docs.microsoft.com/en-gb/windows/win32/api/winbase/nf-winbase-readdirectorychangesw
     */
    const unsigned long _notifyFilter;

    /**
     * \brief if this is a recursive monitoring or not.
     */
    const bool _recursive;

    /**
     * \brief flag to indicate that we received the operation aborted message
     *        and we are able to close the file handle now.
     */
    std::atomic<bool> _operationAborted;

    /**
     * \brief the handle of the directory, (and sub-directory)
     */
    void* _hDirectory;

    /**
     * \brief the buffer that we read
     */
    unsigned char* _buffer;

    /**
     * \brief the buffer length
     */
    const unsigned long _bufferLength;

    /**
     * \brief the path
     */
    const std::wstring _path;

    /// <summary>
    /// The id of the parent monitor
    /// </summary>
    const long long _id;

    /// <summary>
    /// The overlapped structure used to listen for changes.
    /// </summary>
    OVERLAPPED_DATA* _overlapped = nullptr;

    /// <summary>
    /// Flag used to tell if we want to stop everything or not
    /// Also used to prevent restartint
    /// </summary>
    enum class CollectionState
    {
      Unknown,
      Started,
      Stopped
    };
    CollectionState _colectionState = CollectionState::Unknown;
    #pragma endregion

    #pragma region Clearup
    /// <summary>
    /// Clear all the data that is left in the vector
    /// </summary>
    void clear_data();

    /**
     * \brief Clear the handle
     */
    void clear_handle();

    /**
     * \brief clear the buffer data.
     */
    void clear_buffer();

    /**
     * \brief clear the overlapped structure.
     */
    void clear_overlapped();
    #pragma endregion
  };
}
