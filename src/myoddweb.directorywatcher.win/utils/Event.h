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
      std::wstring Path;

      /**
       * \brief Extra information, (used for rename and so on).
       */
      std::wstring Extra;

      /**
       * \brief the action.
       */
      int Action;

      /**
       * \brief when the event happened in ms
       */
      long long TimeMillisecondsUtc;
    };
  }
}
