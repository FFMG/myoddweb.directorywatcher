// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
using System;
using System.Runtime.CompilerServices;
using myoddweb.directorywatcher.interfaces;

[assembly: InternalsVisibleTo("myoddweb.directorywatcher.test")]
namespace myoddweb.directorywatcher.utils
{
  /// <inheritdoc />
  internal class EventError : IEventError
  {
    /// <summary>
    /// The on-demand created message
    /// This is only created if the caller calls Message{get;}
    /// </summary>
    private string _message;

    /// <inheritdoc />
    public DateTime DateTimeUtc { get; }

    /// <inheritdoc />
    public interfaces.EventError Code { get; }

    /// <inheritdoc />
    public string Message => _message ?? (_message = CreateMessage());

    public EventError(interfaces.EventError error, DateTime utc )
    {
      // the date of the event.
      DateTimeUtc = utc;

      // checkthat the given enum value is valid.
      if (!Enum.IsDefined(typeof(interfaces.EventError), error))
      {
        throw new ArgumentOutOfRangeException(nameof(error), error, "Unknown Event Error code.");
      }
      Code = error;
    }

    /// <summary>
    /// Create an error message given the code.
    /// </summary>
    /// <returns></returns>
    private string CreateMessage()
    {
      switch (Code)
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
          return $"An unknown error code was returned: {(int)Code}";
      }
    }
  }
}
