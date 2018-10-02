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
  public interface IWatcher1
  {
    /// <summary>
    /// The path we wish to monitor for changes
    /// </summary>
    /// <param name="path">The path we want to monitor.</param>
    /// <param name="recursive"></param>
    /// <returns>Unique Id used to release/stop monitoring</returns>
    long Start(string path, bool recursive);

    /// <summary>
    /// Stop monitoring a path
    /// </summary>
    /// <param name="id"></param>
    /// <returns></returns>
    bool Stop(long id);
  }
}
