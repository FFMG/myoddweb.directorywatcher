// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include <Windows.h>
#include "../Monitor.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    namespace win
    {
      class Data final
      {
      public:
        /**
         * \brief callback function when we get a notification.
         */
        typedef std::function<void(unsigned char*)> DataCallbackFunction;

        explicit Data(
          const Monitor& monitor, 
          unsigned long notifyFilter,
          bool recursive,
          DataCallbackFunction& dataCallbackFunction,
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
        bool StartMonitoring();

        /**
         * \brief Clear all the data
         */
        void StopMonitoring();

        /**
         * \brief check that he current handle is still valie
         *        if not then we will close the connection.
         */
        void CheckStillValid();
      private:

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
         * \brief get the current buffer
         * \return the void* buffer.
         */
        [[nodiscard]]
        void* Buffer() const;

        /**
         * \brief get the buffer length
         * \return the buffer length
         */
        [[nodiscard]]
        unsigned long BufferLength() const;

        /**
         * \brief Get pointer to our overlapped data.
         * \return pointer to the 'OVERLAPPED' struct.
         */
        LPOVERLAPPED Overlapped();

      private:
        /**
         * \brief the number of times we had an invalid handle.
         *        after a certain count we will close this.
         */
        int _invalidHandleWait;

        /**
         * \brief start monitoring a given folder.
         * \return success or not
         */
        bool BeginRead();

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
        bool _operationAborted;

        /***
         * \brief the completion routine the caller of Start( ... ) would like called when an event is raised.
         */
        DataCallbackFunction& _dataCallbackFunction;

        static void __stdcall FileIoCompletionRoutine(
          unsigned long dwErrorCode,							  // completion code
          unsigned long dwNumberOfBytesTransfered,	// number of bytes transferred
          _OVERLAPPED* lpOverlapped                 // I/O information buffer
        );

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
        unsigned long _bufferLength;

        /**
         * \brief the path
         */
        const Monitor& _monitor;

        /**
         * \brief the overlapped
         */
        OVERLAPPED	_overlapped{};

        #pragma region Clearup
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
  }
}
