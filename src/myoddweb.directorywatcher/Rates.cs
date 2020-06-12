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

    public Rates(long statisticsMilliseconds, long eventsMilliseconds)
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
