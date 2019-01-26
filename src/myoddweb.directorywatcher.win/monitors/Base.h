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

namespace myoddweb
{
  namespace directorywatcher
  {
    /**
     * The min number of Milliseconds we want to wait.
     * If this number is too low then we will use more CPU.
     */
    constexpr auto MYODDWEB_MIN_THREAD_SLEEP = 2L;

    /*
     * The maximum number of Milliseconds we want to sleep.
     * If this number is too low then we will use more CPU
     * But if it is too high then shutdown could be really slow.
     */
    constexpr auto  MYODDWEB_MAX_THREAD_SLEEP = 256L;

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
