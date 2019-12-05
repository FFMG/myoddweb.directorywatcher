// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include <Windows.h>
#include "../Monitor.h"

#define MY_TRACE(msg, ...) MyTraceImpl(__LINE__, __FILE__, msg, __VA_ARGS__)

// implementation us
static void MyTraceImpl(int line, const char* fileName, const char* msg, ...)
{
  va_list args;
  char buffer[256] = { 0 };
//  sprintf_s(buffer, "%s(%d) : ", fileName, line);
//  OutputDebugStringA(buffer);

  // retrieve the variable arguments
  va_start(args, msg);
  vsprintf_s(buffer, msg, args);
  OutputDebugStringA(buffer);
  va_end(args);
}

namespace myoddweb
{
  namespace directorywatcher
  {
    namespace win
    {
      class Data
      {
      public:
        typedef
          void
          (__stdcall *_COMPLETION_ROUTINE)(
            unsigned long mumberOfBytesTransfered,
            void* object,
            Data& data
            );

      public:
        explicit Data(const Monitor& monitor, unsigned long bufferLength);
        ~Data();

        /**
         * \brief Prevent copy construction
         */
        Data(const Data&) = delete;
        Data(Data&&) = delete;
        Data& operator=(const Data&) = delete;
        Data& operator=(Data&& other) = delete;
        Data() = delete;

      public:
        /**
         * \brief Clear all the data
         */
        void Close();

        /**
         * \brief Check if the handle is valid
         */
        bool IsValidHandle() const;

        /**
         * \brief get the directory handle, if we have one.
         * \return the handle
         */
        void* DirectoryHandle() const;

        /**
         * \brief set the directory handle
         * \return if success or not.
         */
        bool Open();

        /**
         * \brief if there was a problem, try re-open the file.
         * \return if success or not.
         */
        bool TryReopen();

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

        /**
         * \brief start monitoring a given folder.
         * \param object the object we will be passed by the overlapped param
         * \param notifyFilter the notification filter, (what we are watching the folder for)
         * \param recursive recursively check the given folder or not.
         * \param lpCompletionRoutine the completion routine we will call.
         * \return success or not
         */
        bool Start(void* object, unsigned long notifyFilter, bool recursive, const _COMPLETION_ROUTINE& lpCompletionRoutine);

      private:
        /**
         * \brief Prepare the buffer and structure for processing.
         * \param object the object we would like to pass to the `OVERLAPPED` structure.
         * \param bufferLength the lenght of the buffer.
         * \param lpCompletionRoutine the routine we will be calling when we get a valid notification
         */
        void PrepareMonitor(void* object, unsigned long bufferLength, const _COMPLETION_ROUTINE& lpCompletionRoutine);

        /***
         * \brief the completion routine the caller of Start( ... ) would like called when an event is raised.
         */
        _COMPLETION_ROUTINE _lpCompletionRoutine;

        /***
         * \brief the object that the caller of Start(...) would like to pass back.
         */
        void* _object;

        /**
         * \brief if we are not watching for folder deletion, we will create our own watcher here
         *        to monitor the folder being deleted.
         */
        Data* _folder;

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
         * \brief if we opened the folder data, close it now.
         */
        void CloseFolder();

        /**
         * \brief clear the overlapped structure.
         */
        void ClearOverlapped();
        #pragma endregion 
      };
    }
  }
}
