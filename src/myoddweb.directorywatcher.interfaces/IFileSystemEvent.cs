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