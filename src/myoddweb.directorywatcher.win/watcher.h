// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include "utils/Request.h"
#include <vector>
#include "utils/Event.h"
#include "monitors/Callbacks.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    /**
     * \brief Start watching a folder
     * \param request The request containing info about the item we are watching.
     * \return The id of the created request or -ve otherwise
     */
    extern "C" { __declspec(dllexport) long long Start( const Request& request ); }

    /**
     * \brief stop watching
     * \param id the id we would like to remove.
     * \return success or not
     */
    extern "C" { __declspec(dllexport) bool Stop(long long id ); }
  }
}
