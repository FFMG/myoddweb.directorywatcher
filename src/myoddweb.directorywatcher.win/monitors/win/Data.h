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
#include <Windows.h>
#include "../Monitor.h"

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
          ( *_COMPLETION_ROUTINE)(
            unsigned long errorCode,
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
         */
        bool Open();

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

        bool Start(void* object, unsigned long notifyFilter, bool recursive, _COMPLETION_ROUTINE lpCompletionRoutine);

      private:
        /**
         * \brief Prepare the buffer and structure for processing.
         * \param object the object we would like to pass to the `OVERLAPPED` structure.
         * \param bufferLength the lenght of the buffer.
         */
        void PrepareMonitor(void* object, unsigned long bufferLength, _COMPLETION_ROUTINE lpCompletionRoutine);

        _COMPLETION_ROUTINE _lpCompletionRoutine;

        void* _object;

        Data* _folder;

        static void CALLBACK FileIoCompletionRoutine(
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
        void CloseFolder() const;

        /**
         * \brief clear the overlapped structure.
         */
        void ClearOverlapped();
        #pragma endregion 
      };
    }
  }
}
