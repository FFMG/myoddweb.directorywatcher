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
    NoFileData = 7
  }
}
