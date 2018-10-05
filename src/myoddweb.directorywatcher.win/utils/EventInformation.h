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

enum class ManagedEventAction
{
  /// <summary>
  /// General error
  /// </summary>
  Error = 0,

  /// <summary>
  /// General memory error, (out of and so on).
  /// </summary>
  ErrorMemory = 1,

  /// <summary>
  /// there was an overflow.
  /// </summary>
  ErrorOverflow = 2,

  /// <summary>
  /// the monitoring was stopped somehow.
  /// </summary>
  ErrorAborted = 3,

  /// <summary>
  /// Unable to even start the monitoring
  /// Is the path valid? Is the filename valid?
  /// </summary>
  ErrorCannotStart = 4,

  /// <summary>
  /// Cannot access the file/folder
  /// </summary>
  ErrorAccess = 5,

  /// <summary>
  /// We have an unknown file error.
  /// </summary>
  Unknown = 1000,
  Added = 1001,
  Removed = 1002,

  /// <summary>
  /// Small changed, timestamp, attribute etc...
  /// </summary>
  Touched = 1003,
  Renamed = 1004
};

enum class EventAction
{
  /**
   * \brief There was a general error
   */
  Error = 0,

  /**
   * \brief general memory error, (out of and so on).
   */
  ErrorMemory = 1,

  /**
   * \brief there was an overflow.
   */
  ErrorOverflow = 2,

  /**
   * \brief the monitoring was stopped somehow.
   */
  ErrorAborted = 3,

  /**
   * \brief Unable to even start the monitoring
   * Is the path valid? Is the filename valid?
   */
  ErrorCannotStart = 4,

  /**
   * \Brief cannot access the file/folder
   */
  ErrorAccess = 5,

  /**
   * We have an unknown file error.
   */
  Unknown = 1000,
  Added = 1001,
  Removed = 1002,

  /**
   * \brief small changed, timestamp, attribute etc...
   */
  Touched = 1003,
  Renamed = 1004
};

/**
 * \brief Information about a file/folder event.
 */
struct EventInformation
{
  /**
   * \brief the time in Ms when this event was recorded.
   */
  long long timeMillisecondsUtc{};

  /**
   * \brief the action we are recording
   */
  EventAction action;

  /**
   * \brief the filename/folder that was updated. 
   */
  std::wstring name;

  /**
   * \brief the old name in the case of a rename.
   */
  std::wstring oldname;
};
