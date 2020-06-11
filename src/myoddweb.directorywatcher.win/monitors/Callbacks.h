// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once

namespace myoddweb:: directorywatcher
{
  /**
   * \brief various statistics
   * \param elapsedTime the number of ms since the last time this was called.
   * \param numberOfEvents the number of events since the last time.
   * \param actualNumberOfMonitors the actual number of sub-monitors.
   */
  typedef void(__stdcall* StatsCallback)(
    long long elapsedTime,
    long long numberOfEvents,
    long long actualNumberOfMonitors
    );

  /**
   * \brief the callback function when an event is raised.
   */
  typedef int(__stdcall *EventCallback)(
    long long id,
    bool isFile,
    const wchar_t* name,
    const wchar_t* oldName,
    int action,
    int error,
    long long dateTimeUtc
    );
}