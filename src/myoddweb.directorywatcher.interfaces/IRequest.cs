// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
namespace myoddweb.directorywatcher.interfaces
{
  public interface IRequest
  {
    /// <summary>
    /// The path we are watching
    /// </summary>
    string Path { get; }

    /// <summary>
    /// If it is recursive or not.
    /// </summary>
    bool Recursive { get; }
  }
}