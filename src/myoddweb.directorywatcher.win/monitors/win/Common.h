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
      protected:
        Common( Monitor& parent, unsigned long bufferLength);

      public:
        /**
         * \brief prevent copies.
         */
        Common() = delete;
        Common(const Common&) = delete;
        Common(Common&&) = delete;
        Common& operator=(const Common&) = delete;
        Common& operator=(Common&&) = delete;

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

        static void __stdcall FileIoCompletionRoutine(
          unsigned long mumberOfBytesTransfered,
          void* object,
          Data& data
          );

        static void RunThread(Common* obj);

      private:
        void StopAndResetThread();

        void ProcessNotificationFromBackup(const unsigned char* pBuffer) const;

        void Read();
        void Run();

        /**
         * \brief check if we have to stop the current work.
         * \return bool if we have to stop or not.
         */
        bool MustStop() const;

        /**
         * \brief if this value is true, then we will force a stop.
         *        this includes all the looks and so on.
         */
        bool _mustStop;

        /**
         * \brief all the data used by the monitor.
         */
        Data* _data;

        /**
         * \brief the parent monitor
         */
        Monitor& _parent;

        /**
         * \brief the max length of the buffers.
         */
        const unsigned long _bufferLength;
        
        #pragma region Thread variables
        /**
         * \brief signal to stop the thread.
         */
        std::promise<void> _exitSignal;
        std::future<void> _futureObj;
        std::thread* _th = nullptr;
        #pragma endregion

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