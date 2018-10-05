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
    public EventAction Code { get; }

    /// <inheritdoc />
    public string Message { get; }

    public EventError(EventAction action, DateTime utc )
    {
      DateTimeUtc = utc;
      Code = action;
      Message = CreateMessage(action);
    }

    /// <summary>
    /// Create an error message given the code.
    /// </summary>
    /// <param name="action"></param>
    /// <returns></returns>
    internal static string CreateMessage(EventAction action)
    {
      switch (action)
      {
        case EventAction.Error:
          return "General error";

        case EventAction.ErrorMemory:
          return "Guarded risk of memory corruption";

        case EventAction.ErrorOverflow:
          return "Guarded risk of memory overflow";

        case EventAction.ErrorAborted:
          return "Monitoring was aborted";

        case EventAction.ErrorCannotStart:
          return "Unable to start monitoring";

        case EventAction.ErrorAccess:
          return "Unable to access the given file/folder";

        case EventAction.Unknown:
          return "An unknown error occured";

        default:
          return $"An unknown error code was returned: {(int)action}";
      }
    }
  }
}
