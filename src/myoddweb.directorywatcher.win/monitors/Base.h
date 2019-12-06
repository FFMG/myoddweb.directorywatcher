// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once

namespace myoddweb
{
  namespace directorywatcher
  {
    /**
     * The min number of Milliseconds we want to wait.
     * If this number is too low then we will use more CPU.
     */
    constexpr auto MYODDWEB_MIN_THREAD_SLEEP = 2L;

    /**
     * When a handle becomes invalid, (the watched folder was deleted)
     * How often do we want to re-check the folder and, if valid
     * How often do we want to try and re-open it.
     */
    constexpr auto  MYODDWEB_INVALID_HANDLE_SLEEP = 5000L;

    /**
     * The maximum number of subpath we want to allow in multiple windows monitor.
     * If the number is too large the number of running threads will cause issues.
     */
    constexpr auto MYODDWEB_MAX_NUMBER_OF_SUBPATH = 64L;
  }
}
