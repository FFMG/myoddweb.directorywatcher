// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once

namespace myoddweb:: directorywatcher
{
  /// <summary>
  /// The different log levels
  /// </summary>
  enum class LogLevel : int
  {
    /// <summary>
    /// We have an unknown log leve
    /// </summary>
    Unknown = 0,

    /// <summary>
    /// Informational message
    /// </summary>
    Information = 1,

    /// <summary>
    /// This is just a warning, something unimportant happned
    /// </summary>
    Warning = 2,

    /// <summary>
    /// There was an error that prevented an acction to be completed.
    /// </summary>
    Error = 3,

    /// <summary>
    /// Something terrible happened! Might cause program to end
    /// </summary>
    Panic = 4,

    /// <summary>
    /// Only debug messages are included here.
    /// </summary>
    Debug = 100
  };
}
