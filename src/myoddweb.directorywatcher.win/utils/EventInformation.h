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
      ManagedEventAction Action;

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