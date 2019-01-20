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
using myoddweb.directorywatcher.interfaces;

namespace myoddweb.directorywatcher.utils
{
  /// <inheritdoc />
  internal class EventError : IEventError
  {
    /// <inheritdoc />
    public DateTime DateTimeUtc { get; }

    /// <inheritdoc />
    public interfaces.EventError Code { get; }

    /// <inheritdoc />
    public string Message { get; }

    public EventError(interfaces.EventError error, DateTime utc )
    {
      DateTimeUtc = utc;
      Code = error;
      Message = CreateMessage(error);
    }

    /// <summary>
    /// Create an error message given the code.
    /// </summary>
    /// <param name="action"></param>
    /// <returns></returns>
    internal static string CreateMessage(interfaces.EventError action)
    {
      switch (action)
      {
        case interfaces.EventError.General:
          return "General error";

        case interfaces.EventError.Memory:
          return "Guarded risk of memory corruption";

        case interfaces.EventError.Overflow:
          return "Guarded risk of memory overflow";

        case interfaces.EventError.Aborted:
          return "Monitoring was aborted";

        case interfaces.EventError.CannotStart:
          return "Unable to start monitoring";

        case interfaces.EventError.Access:
          return "Unable to access the given file/folder";

        case interfaces.EventError.NoFileData:
          return "The raised event did not have any valid file name";

        case interfaces.EventError.CannotStop:
          return "There was an issue trying to stop the watcher(s)";

        case interfaces.EventError.None:
          return "No Error";

        default:
          return $"An unknown error code was returned: {(int)action}";
      }
    }
  }
}
