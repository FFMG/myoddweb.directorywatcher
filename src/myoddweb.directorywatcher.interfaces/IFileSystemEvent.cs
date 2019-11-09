// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
using System;
using System.IO;

namespace myoddweb.directorywatcher.interfaces
{
  public interface IFileSystemEvent
  {
    /// <summary>
    /// The file system event.
    /// </summary>
    FileSystemInfo FileSystemInfo { get; }

    /// <summary>
    ///  Gets the full path of the directory or file.
    /// </summary>
    /// <returns>A string containing the full path.</returns>
    string FullName { get; }

    /// <summary>
    ///     For files, gets the name of the file. For directories, gets the name of the last
    ///     directory in the hierarchy if a hierarchy exists. Otherwise, the Name property
    ///     gets the name of the directory.
    /// </summary>
    /// <returns>A string that is the name of the parent directory, the name of the last directory
    ///     in the hierarchy, or the name of a file, including the file name extension.
    /// </returns>
    string Name { get; }

    /// <summary>
    /// The Action
    ///  Added
    ///  Removed
    ///  Touched
    ///  Renamed
    /// </summary>
    EventAction Action { get; }

    /// <summary>
    /// If there was an error or not.
    /// </summary>
    EventError Error { get; }

    /// <summary>
    /// The UTC date time of the event.
    /// </summary>
    DateTime DateTimeUtc { get; }

    /// <summary>
    /// Boolean if the update is a file or a directory.
    /// </summary>
    bool IsFile { get; }

    /// <summary>
    /// Return if the event is a certain action
    /// (same as Action == action)
    /// </summary>
    /// <param name="action"></param>
    /// <returns></returns>
    bool Is(EventAction action );
  }
}