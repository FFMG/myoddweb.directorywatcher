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

enum class EventAction
{
  Error,      // there was a general error.

  /**
   * \brief general memory error, (out of and so on).
   */
  ErrorMemory,

  /**
   * \brief there was an overflow.
   */
  ErrorOverflow,

  /**
   * \brief the monitoring was stopped somehow.
   */
  ErrorAborted,

  /**
   * \brief Unable to even start the monitoring
   * Is the path valid? Is the filename valid?
   */
  ErrorCannotStart,

  /**
   * We have an unknown file error.
   */
  Unknown   = 1000,
  Added,
  Removed,
  Touched,    //  small changed, timestamp, attribute etc...
  RenamedOld,
  RenamedNew
};

/**
 * \brief Information about a file/folder event.
 */
struct EventInformation
{
  /**
   * \brief the time in Ms when this event was recorded.
   */
  __int64 timeMs{};

  /**
   * \brief the action we are recording
   */
  EventAction action;

  /**
   * \brief the filename/folder that was updated. 
   */
  std::wstring name;
};
