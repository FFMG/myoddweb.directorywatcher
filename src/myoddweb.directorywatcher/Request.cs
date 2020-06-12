// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
using System;
using myoddweb.directorywatcher.interfaces;

namespace myoddweb.directorywatcher
{
  public class Request : IRequest
  {
    /// <inheritdoc />
    public string Path { get; }

    /// <inheritdoc />
    public bool Recursive { get; }

    /// <inheritdoc />
    public IRates Rates { get; }

    /// <summary>
    /// Create the default requests
    /// </summary>
    /// <param name="path">The path we want to watch</param>
    /// <param name="recursive">Recursively watch or not.</param>
    public Request(string path, bool recursive) : 
      this( path, recursive, new Rates( 50, 0 ))
    {
    }

    /// <summary>
    /// Create the default requests
    /// </summary>
    /// <param name="path">The path we want to watch</param>
    /// <param name="recursive">Recursively watch or not.</param>
    /// <param name="rates">The various refresh rates</param>
    public Request(string path, bool recursive, IRates rates )
    {
      Path = path ?? throw new ArgumentNullException(nameof(path));
      Recursive = recursive;
      Rates = rates ?? throw new ArgumentNullException(nameof(rates));
    }

  }
}
