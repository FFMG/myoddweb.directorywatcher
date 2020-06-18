// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
using System;
using System.Runtime.CompilerServices;
using myoddweb.directorywatcher.interfaces;

[assembly: InternalsVisibleTo("myoddweb.directorywatcher.test, PublicKey = " +
                              "0024000004800000940000000602000000240000525341310004000001"+
                              "000100e9f9ed09148922c9f0f9faaed99179b7d5be3b4dbaa3777c4aa5"+
                              "9138e8396afe1292f7be40fabb7d11407915b9e45800295ce2f75414fe"+
                              "d968b1fb371ba633e2e4ad46d655016e0e1203d184e1536b8378f7cff9"+
                              "b92b491520172219c7376997588432e32da907f48bbfe6b25aecd7066b"+
                              "2fbd0a88e077d2026a017188af05c7")]
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
