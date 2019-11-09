// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
using System;

namespace myoddweb.directorywatcher.interfaces
{
  public interface IEventError
  {
    /// <summary>
    /// The error code.
    /// </summary>
    EventError Code { get; }

    /// <summary>
    /// The error message
    /// </summary>
    string Message { get; }

    /// <summary>
    /// The UTC date time.
    /// </summary>
    DateTime DateTimeUtc { get; }
  }
}