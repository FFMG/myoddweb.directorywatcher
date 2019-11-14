// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
using System;
using System.IO;
using myoddweb.directorywatcher.interfaces;

namespace myoddweb.directorywatcher.utils
{
  /// <inheritdoc />
  internal class FileSystemEvent : IFileSystemEvent
  {
    /// <inheritdoc />
    public FileSystemInfo FileSystemInfo { get; }

    /// <inheritdoc />
    public EventAction Action { get; }

    /// <inheritdoc />
    public interfaces.EventError Error { get; }

    /// <inheritdoc />
    public DateTime DateTimeUtc { get; }

    /// <inheritdoc />
    public bool IsFile => FileSystemInfo is FileInfo;

    /// <inheritdoc />
    public string FullName => FileSystemInfo?.FullName ?? "???";

    /// <inheritdoc />
    public string Name => FileSystemInfo?.Name ?? "???";

    public FileSystemEvent( IEvent e )
    {
      Action = e.Action;
      Error = e.Error;
      if (e.IsFile)
      {
        FileSystemInfo = new FileInfo(e.Name);
      }
      else
      {
        FileSystemInfo = new DirectoryInfo(e.Name);
      }
      DateTimeUtc = e.DateTimeUtc;
    }

    /// <inheritdoc />
    public bool Is(EventAction action)
    {
      return Action == action;
    }
  }
}
