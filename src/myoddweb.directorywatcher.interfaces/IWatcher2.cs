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
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;

namespace myoddweb.directorywatcher.interfaces
{
  public delegate Task WatcherEvent<in T>(T e, CancellationToken token);

  public interface IWatcher2 : IWatcher1
  {
    #region Events
    /// <summary>
    /// Event when a FileSystem event was 'touched'.
    /// Changed attribute, size changed etc...
    /// </summary>
    event WatcherEvent<IFileSystemEvent> OnTouchedAsync;

    /// <summary>
    /// Event when a FileSystem event was added.
    /// </summary>
    event WatcherEvent<IFileSystemEvent> OnAddedAsync;

    /// <summary>
    /// Event when a FileSystem event was Removed.
    /// </summary>
    event WatcherEvent<IFileSystemEvent> OnRemovedAsync;

    /// <summary>
    /// Event when a FileSystem event was Renamed.
    /// </summary>
    event WatcherEvent<IRenamedFileSystemEvent> OnRenamedAsync;

    /// <summary>
    /// There was an error.
    /// </summary>
    event WatcherEvent<IEventError> OnErrorAsync;
    #endregion

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