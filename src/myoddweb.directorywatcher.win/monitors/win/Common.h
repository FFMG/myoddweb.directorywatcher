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
#include "Data.h"
#include <future>
#include "../../utils/EventAction.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    namespace win
    {
      class Common
      {
      public:
        Common(const Monitor& parent, unsigned long bufferLength);

      private:
        /**
         * \brief prevent copies.
         */
        Common(const Common&) = delete;
        Common& operator=(const Common&) = delete;

      public:
        virtual ~Common();

        virtual bool Start();
        virtual void Stop();

      protected:
        /**
         * \brief Get the notification filter.
         * \return the notification filter
         */
        virtual unsigned long GetNotifyFilter() const = 0;

        static void CALLBACK FileIoCompletionRoutine(
          unsigned long dwErrorCode,							  // completion code
          unsigned long dwNumberOfBytesTransfered,	// number of bytes transferred
          _OVERLAPPED* lpOverlapped                 // I/O information buffer
        );

        static void RunThread(Common* obj);

      private:
#pragma region
        void Reset();
        void StopAndResetThread();
#pragma endregion Reset/Stop functions

        bool OpenDirectory();
        void ProcessNotificationFromBackup(const unsigned char* pBuffer) const;

        void Read();
        void Run();

        /**
         * \brief all the data used by the monitor.
         */
        Data _data;

        /**
         * \brief the parent monitor
         */
        const Monitor& _parent;
#pragma region
        /**
         * \brief signal to stop the thread.
         */
        std::promise<void> _exitSignal;
        std::future<void> _futureObj;
        std::thread* _th;
#pragma endregion Thread variables

        void StartWorkerThread();

      protected:
        /**
         * \brief check if a given string is a file or a directory.
         * \param action the action we are looking at
         * \param path the file we are checking.
         * \return if the string given is a file or not.
         */
        virtual bool IsFile(EventAction action, const std::wstring& path) const;
      };
    }
  }
}