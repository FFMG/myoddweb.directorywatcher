// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
namespace myoddweb.directorywatcher.interfaces
{
  public enum EventAction
  {
    /// <summary>
    /// We have an unknown file event.
    /// </summary>
    Unknown = 1000,

    /// <summary>
    /// A file/directory was added
    /// </summary>
    Added = 1001,

    /// <summary>
    /// A file directory was removed
    /// </summary>
    Removed = 1002,

    /// <summary>
    /// Small changed, timestamp, attribute etc...
    /// </summary>
    Touched = 1003,

    /// <summary>
    /// A file/directory was renamed.
    /// @see IRenamedFileSystemEvent for the old/new name.
    /// </summary>
    Renamed = 1004
  }
}