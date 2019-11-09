// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
using System;

namespace myoddweb.directorywatcher.interfaces
{
  public interface IEvent
  {
    /// <summary>
    /// Boolean if the update is a file or a directory.
    /// </summary>
    bool IsFile { get; }

    /// <summary>
    /// The full path
    /// </summary>
    string Name { get; }

    /// <summary>
    /// In the case of a rename, this is the old name
    /// </summary>
    string OldName { get; }

    /// <summary>
    /// The action defining this event.
    /// </summary>
    EventAction Action { get; }

    /// <summary>
    /// If there was an error with this event
    /// EventError.None otherwise
    /// </summary>
    EventError Error { get; }

    /// <summary>
    /// When the event happened.
    /// </summary>
    DateTime DateTimeUtc { get; }
  }
}