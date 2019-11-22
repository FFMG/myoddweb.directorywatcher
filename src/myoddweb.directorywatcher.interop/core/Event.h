// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
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
  const wchar_t* Name;

  /**
   * \brief In the case of rename, this is the old name.
   */
  const wchar_t* OldName;

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
