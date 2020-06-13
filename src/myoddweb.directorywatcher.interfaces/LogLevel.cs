// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
namespace myoddweb.directorywatcher.interfaces
{
  /// <summary>
  /// The various log levels.
  /// </summary>
  /// NB: We force the type int to make it clear that the enum is an int
  ///     This is what the unmanaged class is passing us!
  // ReSharper disable once EnumUnderlyingTypeIsInt
  public enum LogLevel : int
  {
    /// <summary>
    /// We have an unknown log level
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
  }
}