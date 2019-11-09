// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include "../Monitor.h"
#include "Common.h"
#include "../../utils/EventAction.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    namespace win
    {
      class Files : public Common
      {
      public:
        Files( Monitor& parent, unsigned long bufferLength);

      public:
        virtual ~Files() = default;

      private:
        /**
         * \brief prevent copies.
         */
        Files(const Files&) = delete;
        Files& operator=(const Files&) = delete;

      protected:
        /**
         * Get the notification filter.
         * \return the notification filter
         */
        unsigned long GetNotifyFilter() const override;

        /**
         * \brief check if a given string is a file or a directory.
         * \param action the action we are looking at
         * \param path the file we are checking.
         * \return if the string given is a file or not.
         */
        bool IsFile(EventAction action, const std::wstring& path) const override;
      };
    }
  }
}
