// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
using System.IO;
using myoddweb.directorywatcher.interfaces;

namespace myoddweb.directorywatcher.utils
{
  /// <inheritdoc cref="IRenamedFileSystemEvent" />
  internal class RenamedFileSystemEvent : FileSystemEvent, IRenamedFileSystemEvent
  {
    /// <inheritdoc />
    public FileSystemInfo PreviousFileSystemInfo { get; }

    /// <inheritdoc />
    public string PreviousFullName => PreviousFileSystemInfo?.FullName ?? "???";

    /// <inheritdoc />
    public string PreviousName => PreviousFileSystemInfo?.Name ?? "???";

    public RenamedFileSystemEvent(IEvent e) : base(e)
    {
      PreviousFileSystemInfo = new FileInfo(e.OldName);
    }
  }
}
