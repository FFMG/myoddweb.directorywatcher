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