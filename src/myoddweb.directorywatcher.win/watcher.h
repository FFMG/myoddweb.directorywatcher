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
#include "utils/Request.h"
#include <vector>
#include "utils/Event.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    /**
     * \brief Start watching a folder
     * \param request The request containing info about the item we are watching.
     * \return The id of the created request or -ve otherwise
     */
    extern "C" { __declspec(dllexport) long long Start( Request request ); }

    /**
     * \brief stop watching
     * \param id the id we would like to remove.
     * \return success or not
     */
    extern "C" { __declspec(dllexport) bool Stop(long long id ); }

    /**
     * \brief Get the latest events.
     * \param id the id of the monitor we would like the events for.
     * \param events the events we will be getting
     * \return the number of items or -ve in case of an error
     */
    extern "C" { __declspec(dllexport) long long GetEvents( long long id, std::vector<Event>& events ); }
  }
}
