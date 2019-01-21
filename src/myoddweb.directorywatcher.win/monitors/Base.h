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
    /*
     * The number of ms we want to sleep between checks.
     * The longer we wait, the more time shutdown will take
     * The shorter we wait, the more CPU will be used.
     */
    #define MYODDWEB_SLEEP_BETWEEN_READS 200L

    /**
     * The maximum number of subpath we want to allow in multiple windows monitor.
     * If the number is too large the number of running threads will cause issues.
     */
    #define MYODDWEB_MAX_NUMBER_OF_SUBPATH 32L

    /***
     * The max number of levels we want to use.
     * The higher the number the higher number of thread
     * The smaller the number, the higher the rist of memory issues.
     */
    #define MYODDWEB_MAX_LEVEL_DEPTH 2
  }
}
