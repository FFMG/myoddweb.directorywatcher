// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include <Windows.h>

#include "Data.h"
#include "../Monitor.h"
#include "../../utils/EventAction.h"
#include "../../utils/Threads/Thread.h"

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

        virtual ~Common();

        bool Start();
        void Update() const;
        void Stop();
      protected:
        /**
         * \brief Get the notification filter.
         * \return the notification filter
         */
        [[nodiscard]]
        virtual unsigned long GetNotifyFilter() const = 0;

      private:
        /**
         * \brief start monitoring the given folder.
         * \return if we managed to start the monitoring or not.
         */
        bool CreateAndStartData();

        void ProcessNotification(const unsigned char* pBuffer) const;

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

      protected:
        /**
         * \brief check if a given string is a file or a directory.
         * \param action the action we are looking at
         * \param path the file we are checking.
         * \return if the string given is a file or not.
         */
        [[nodiscard]]
        virtual bool IsFile(EventAction action, const std::wstring& path) const;
      };
    }
  }
}