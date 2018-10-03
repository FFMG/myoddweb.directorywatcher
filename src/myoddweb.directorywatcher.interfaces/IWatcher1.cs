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
using System.Collections.Generic;

namespace myoddweb.directorywatcher.interfaces
{
  public interface IWatcher1
  {
    /// <summary>
    /// Start a request and return its id.
    /// </summary>
    /// <param name="request"></param>
    /// <returns></returns>
    long Start(IRequest request );

    /// <summary>
    /// Stop and remove a currently running request.
    /// </summary>
    /// <param name="id"></param>
    /// <returns></returns>
    bool Stop(long id );

    /// <summary>
    /// Get the event for the given id.
    /// </summary>
    /// <param name="id">The id we are looking for.</param>
    /// <param name="events">The events we got.</param>
    /// <returns>The number of events</returns>
    long GetEvents(long id, out IList<IEvent> events );
  }
}
