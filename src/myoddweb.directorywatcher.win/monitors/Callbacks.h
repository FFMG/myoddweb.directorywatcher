// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once

namespace myoddweb:: directorywatcher
{
  /**
   * \brief a log message
   * \param id the id of the monitor, (0 if global)
   * \param type the message type, (verbose, information, warning etc ...)
   * \param message the message itself
   */
  typedef void(__stdcall* LoggerCallback)(
    long long id,
    int type,
    const wchar_t* message
    );

  /**
   * \brief various statistics
   * \param id the monitor id
   * \param elapsedTime the number of ms since the last time this was called.
   * \param numberOfEvents the number of events since the last time.
   */
  typedef void(__stdcall* StatisticsCallback)(
    long long id,
    double elapsedTime,
    long long numberOfEvents
    );

  /**
   * \brief the callback function when an event is raised.
   * \param id the monitor id
   * \param isFile if the event is for a file or not.
   * \param name the name of the file
   * \param oldName the previous name of the file, (if needed)
   * \param action the action that happened
   * \param error the error type, (if any)
   * \param dateTimeUtc unix timestamp of the event
   */
  typedef void(__stdcall *EventCallback)(
    long long id,
    bool isFile,
    const wchar_t* name,
    const wchar_t* oldName,
    int action,
    int error,
    long long dateTimeUtc
    );
}