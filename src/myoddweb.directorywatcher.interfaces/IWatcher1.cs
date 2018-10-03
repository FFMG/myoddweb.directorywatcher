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
    /// Add a request to the list of requests.
    /// If we have already started, we will start this request right away.
    /// </summary>
    /// <param name="request"></param>
    /// <returns></returns>
    long Add(IRequest request );

    /// <summary>
    /// Stop and remove a currently running request.
    /// </summary>
    /// <param name="id"></param>
    /// <returns></returns>
    bool Remove(long id );

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
  }
}
