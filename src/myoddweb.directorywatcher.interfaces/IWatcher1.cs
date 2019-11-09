// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
using System.Collections.Generic;

namespace myoddweb.directorywatcher.interfaces
{
  public interface IWatcher1
  {
    /// <summary>
    /// Start a request and return its id.
    /// </summary>
    /// <param name="request"></param>
    /// <returns></returns>
    long Start(IRequest request );

    /// <summary>
    /// Stop and remove a currently running request.
    /// </summary>
    /// <param name="id"></param>
    /// <returns></returns>
    bool Stop(long id );

    /// <summary>
    /// Get the event for the given id.
    /// </summary>
    /// <param name="id">The id we are looking for.</param>
    /// <param name="events">The events we got.</param>
    /// <returns>The number of events</returns>
    long GetEvents(long id, out IList<IEvent> events );
  }
}
