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

/**
 * \brief implementation of IEvent
 */
public ref class Event : myoddweb::directorywatcher::interfaces::IEvent
{
public:
  /**
   * \brief the path of the folder that raised the event.
   */
  virtual property System::String^ Name;

  /**
   * \brief Extra information, mostly null, (for example rename)
   */
  virtual property System::String^ OldName;

  /**
   * \brief The event action.
   */
  virtual property myoddweb::directorywatcher::interfaces::EventAction Action;

  /**
   * \brief The event error.
   */
  virtual property myoddweb::directorywatcher::interfaces::EventError Error;

  /**
   * \brief When the event happened.
   */
  virtual property System::DateTime DateTimeUtc;

  /**
   * \brief Boolean if the update is a file or a directory.
  */
  virtual property bool IsFile;
};

/**
 * \brief unmaned implementation of IEvent
 */
struct UmEvent
{
  /**
   * \brief the path of the folder that raised the event.
   */
  std::wstring Name;

  /**
   * \brief In the case of rename, this is the old name.
   */
  std::wstring OldName;

  /**
   * \brief The action.
   */
  int Action;

  /**
   * \brief The error.
   */
  int Error;

  /**
   * \brief when the event happened in ms
   */
  long long TimeMillisecondsUtc;

  /**
 * \brief Boolean if the update is a file or a directory.
   */
  bool IsFile;
};
