// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include <Windows.h>
#include "../Monitor.h"

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
      unsigned long bufferLength);
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
    bool Start();

    /**
     * \brief Clear all the data
     */
    void Stop();

    std::vector<unsigned char*> Get();

    /**
     * \brief check that he current handle is still valie
     *        if not then we will close the connection.
     */
    void CheckStillValid();
  private:

    MYODDWEB_MUTEX _dataLock;
    std::vector<unsigned char*> _data;

    /**
     * \brief Check if the handle is valid
     */
    [[nodiscard]]
    bool IsValidHandle() const;

    /**
     * \brief set the directory handle
     * \return if success or not.
     */
    bool OpenDirectoryHandle();

    /**
     * \brief start monitoring a given folder.
     * \return success or not
     */
    bool Listen();

    /**
     * \brief prepare the various buffer for changes.
     */
    void PrepareForRead();

    /**
     * \brief process a read received.
     * \param dwNumberOfBytesTransfered the number of bytes received.
     */
    void ProcessRead(unsigned long dwNumberOfBytesTransfered );

    /**
     * \brief process an error code.
     * \param errorCode the error received.
     */
    void ProcessError(unsigned long errorCode);

    /**
     * \brief clone up to 'ulSize' bytes into a buffer.
     *        it is up to the caller to clear/delete the buffer.
     * \param ulSize the max number of bytes we want to copy
     * \return the cloned data.
     */
    [[nodiscard]]
    unsigned char* Clone(unsigned long ulSize) const;

    /// <summary>
    /// The function that will be called when a file event is detected.
    /// </summary>
    /// <param name="dwErrorCode"></param>
    /// <param name="dwNumberOfBytesTransfered"></param>
    /// <param name="lpOverlapped"></param>
    /// <returns></returns>
    static void __stdcall FileIoCompletionRoutine(
      unsigned long dwErrorCode,							  // completion code
      unsigned long dwNumberOfBytesTransfered,	// number of bytes transferred
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
    OVERLAPPED_DATA*	_overlapped;

    bool _stop = true;
    #pragma endregion

    #pragma region Clearup
    /// <summary>
    /// Clear all the data that is left in the vector
    /// </summary>
    void ClearData();

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
    #pragma endregion 
  };
}
