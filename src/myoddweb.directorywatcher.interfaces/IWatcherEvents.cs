// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
namespace myoddweb.directorywatcher.interfaces
{
  public interface IWatcherEvents
  {
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

    /// <summary>
    /// The statistics of the engine.
    /// </summary>
    event WatcherEvent<IStatistics> OnStatisticsAsync;

    /// <summary>
    /// When we get a log message
    /// </summary>
    event WatcherEvent<ILoggerEvent> OnLoggerAsync;
  }
}
