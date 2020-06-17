// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
using System;
using myoddweb.directorywatcher.interfaces;

namespace myoddweb.directorywatcher
{
  public class Rates : IRates
  {
    /// <inheritdoc />
    public long StatisticsMilliseconds { get; }

    /// <inheritdoc />
    public long EventsMilliseconds { get; }

    public Rates(long eventsMilliseconds, long statisticsMilliseconds = 0)
    {
      if (statisticsMilliseconds < 0)
      {
        throw new ArgumentException( "The statistics rate cannot be -ve", nameof(statisticsMilliseconds));
      }
      if (eventsMilliseconds < 0)
      {
        throw new ArgumentException("The events rate cannot be -ve", nameof(eventsMilliseconds));
      }
      StatisticsMilliseconds = statisticsMilliseconds;
      EventsMilliseconds = eventsMilliseconds;
    }
  }
}
