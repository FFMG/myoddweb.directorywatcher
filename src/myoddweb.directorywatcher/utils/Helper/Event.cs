using myoddweb.directorywatcher.interfaces;
using System;

namespace myoddweb.directorywatcher.utils.Helper
{
  internal class Event : IEvent
  {
    public bool IsFile { get; }

    public string Name { get; }

    public string OldName { get; }

    public EventAction Action { get; }

    public interfaces.EventError Error { get; }

    public DateTime DateTimeUtc { get; }

    public Event(bool isFile,
      string name,
      string oldName,
      EventAction action,
      interfaces.EventError error,
      DateTime dateTimeUtc
    )
    {
      IsFile = isFile;
      Name = name;
      OldName = oldName;
      Action = action;
      Error = error;
      DateTimeUtc = dateTimeUtc;
    }
  }
}
