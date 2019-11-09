// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include <string>
#include "EventAction.h"
#include "EventError.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    /**
     * \brief Information about a file/folder event.
     */
    struct EventInformation
    {
      /**
       * \brief the time in Ms when this event was recorded.
       */
      long long TimeMillisecondsUtc{};

      /**
       * \brief the action we are recording
       */
      EventAction Action;

      /**
       * \brief the action we are recording
       */
      EventError Error;

      /**
       * \brief the filename/folder that was updated.
       */
      std::wstring Name;

      /**
       * \brief the old name in the case of a rename.
       */
      std::wstring OldName;

      /**
     * \brief Boolean if the update is a file or a directory.
       */
      bool IsFile;
    };
  }
}