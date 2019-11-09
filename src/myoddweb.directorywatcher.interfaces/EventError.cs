// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
namespace myoddweb.directorywatcher.interfaces
{
  public enum EventError
  {
    /// <summary>
    /// General error
    /// </summary>
    None = 0,

    /// <summary>
    /// General error
    /// </summary>
    General = 1,

    /// <summary>
    /// General memory error, (out of and so on).
    /// </summary>
    Memory = 2,

    /// <summary>
    /// there was an overflow.
    /// </summary>
    Overflow = 3,

    /// <summary>
    /// the monitoring was stopped somehow.
    /// </summary>
    Aborted = 4,

    /// <summary>
    /// Unable to even start the monitoring
    /// Is the path valid? Is the filename valid?
    /// </summary>
    CannotStart = 5,

    /// <summary>
    /// Cannot access the file/folder
    /// </summary>
    Access = 6,

    /// <summary>
    /// The event did not have any file data.
    /// </summary>
    NoFileData = 7,

    /// <summary>
    /// There was an issue trying to stop the watcher(s)
    /// </summary>
    CannotStop = 8
  }
}
