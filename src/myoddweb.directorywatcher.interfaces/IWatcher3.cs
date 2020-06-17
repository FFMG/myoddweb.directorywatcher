// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
using System;
using System.Collections.Generic;

namespace myoddweb.directorywatcher.interfaces
{
  public interface IWatcher3 : IWatcher2, IDisposable
  {
    /// <summary>
    /// If the watcher is ready or not.
    /// </summary>
    /// <returns></returns>
    bool Ready();

    /// <summary>
    /// Get all the statistics for all the events.
    /// </summary>
    /// <param name="statistics">Stats we got.</param>
    /// <returns>The number of statistics</returns>
    long GetStatistics(out IList<IStatistics> statistics);

    /// <summary>
    /// Get the statistics for a watcher
    /// </summary>
    /// <param name="id">The id we are looking for.</param>
    /// <param name="statistics">Stats we got.</param>
    /// <returns>If we got the stats or not.</returns>
    bool GetStatistics(long id, out IStatistics statistics);
  }
}
