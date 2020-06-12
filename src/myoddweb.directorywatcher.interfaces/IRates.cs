// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
namespace myoddweb.directorywatcher.interfaces
{
  /// <summary>
  /// The various rates of refresh
  /// </summary>
  public interface IRates
  {
    /// <summary>
    /// How often we would like the libraries posted
    /// If the value is -1 then the statistics are not published.
    /// </summary>
    long StatisticsMilliseconds { get; }

    /// <summary>
    /// How quickly we want to publish events
    /// If this value is -1 then events are not published, (but still recorded)
    /// </summary>
    long EventsMilliseconds { get; }
  }
}
