// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;

namespace myoddweb.directorywatcher.interfaces
{
  public delegate Task WatcherEvent<in T>(T e, CancellationToken token);

  public interface IWatcher2 : IWatcher1, IWatcherEvents
  {
    /// <summary>
    /// Add a request, if monitoring has started already,
    /// we will start monitoring right away
    /// </summary>
    /// <param name="request"></param>
    /// <returns>Success or not.</returns>
    bool Add(IRequest request);

    /// <summary>
    /// Start all the requests
    /// </summary>
    /// <returns></returns>
    bool Start();

    /// <summary>
    /// Stop all the running requests.
    /// </summary>
    /// <returns></returns>
    bool Stop();

    /// <summary>
    /// Get all the event for the started ids.
    /// </summary>
    /// <param name="events">The events we got.</param>
    /// <returns>The number of events</returns>
    long GetEvents( out IList<IEvent> events);
  }
}