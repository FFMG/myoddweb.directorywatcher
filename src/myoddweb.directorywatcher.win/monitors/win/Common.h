// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include <Windows.h>
#include <future>
#include "Data.h"
#include "../Monitor.h"
#include "../Base.h"
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
        Common(Monitor& parent, unsigned long bufferLength);

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

        void DataCallbackFunction(unsigned char* pBufferBk);

      private:
        void StopAndResetThread();

        void ProcessNotificationFromBackup(const unsigned char* pBuffer) const;

        /**
         * \brief send a request for the data class to read for a single file change.
         */
        void Read();

        /**
         * \brief long running thead to check for folder changes as well as disconnects.
         */
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

        /**
         * \brief the current thread handle, if we have one.
         */
#ifdef MYODDWEB_USE_FUTURE
        std::future<void>* _future;
#else
        std::thread* _thread;
        static void _Run(Common*);
#endif

        /**
         * \brief the callback function.
         */
        Data::DataCallbackFunction* _function;

        /**
         * \brief start the worker thread
         */
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