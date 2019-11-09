// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include <string>

namespace myoddweb
{
  namespace directorywatcher
  {
    /**
     * \brief unmanaged implementation of IEvent
     */
    struct Event
    {
      /**
       * \brief The path that was changed.
       */
      std::wstring Name;

      /**
       * \brief Extra information, (used for rename and so on).
       */
      std::wstring OldName;

      /**
       * \brief the action.
       */
      int Action;

      /**
       * \brief the error.
       */
      int Error;

      /**
       * \brief when the event happened in ms
       */
      long long TimeMillisecondsUtc;

      /**
     * \brief Boolean if the update is a file or a directory.
       */
      bool IsFile;
    };
  }
}
