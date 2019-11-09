// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
using myoddweb.directorywatcher.interfaces;

namespace myoddweb.directorywatcher
{
  public class Request : IRequest
  {
    /// <summary>
    /// 
    /// </summary>
    /// <param name="path">The path we want to watch</param>
    /// <param name="recursive">Recursively watch or not.</param>
    public Request(string path, bool recursive)
    {
      Path = path;
      Recursive = recursive;
    }

    /// <inheritdoc />
    public string Path { get; }

    /// <inheritdoc />
    public bool Recursive { get; }
  }
}
