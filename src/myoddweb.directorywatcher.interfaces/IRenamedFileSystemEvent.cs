// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
using System.IO;

namespace myoddweb.directorywatcher.interfaces
{
  public interface IRenamedFileSystemEvent : IFileSystemEvent
  {
    /// <summary>
    /// The file system event.
    /// </summary>
    FileSystemInfo PreviousFileSystemInfo { get; }

    /// <summary>
    ///  Gets the full path of the directory or file.
    /// </summary>
    /// <returns>A string containing the full path.</returns>
    string PreviousFullName { get; }

    /// <summary>
    ///     For files, gets the name of the file. For directories, gets the name of the last
    ///     directory in the hierarchy if a hierarchy exists. Otherwise, the Name property
    ///     gets the name of the directory.
    /// </summary>
    /// <returns>A string that is the name of the parent directory, the name of the last directory
    ///     in the hierarchy, or the name of a file, including the file name extension.
    /// </returns>
    string PreviousName { get; }
  }
}