// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once

namespace myoddweb
{
  namespace directorywatcher
  {
    namespace threads
    {
      enum class WaitResult
      {
        // names for timed wait function returns
        complete,
        timeout,
      };
    }
  }
}