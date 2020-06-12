using System;
using myoddweb.directorywatcher.interfaces;

namespace myoddweb.directorywatcher.utils.Helper
{
  internal class LoggerEvent : ILoggerEvent
  {
    /// <inheritdoc />
    public long Id { get; }

    /// <inheritdoc />
    public int Type { get; }

    /// <inheritdoc />
    public string Message { get; }

    /// <summary>
    /// The constructor
    /// </summary>
    /// <param name="id"></param>
    /// <param name="type"></param>
    /// <param name="message"></param>
    public LoggerEvent(long id, int type, string message)
    {
      Id = id;
      Type = type;
      Message = message ?? throw new ArgumentNullException(nameof(message));
    }
  }
}
