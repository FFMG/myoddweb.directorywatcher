using System;
using myoddweb.directorywatcher.interfaces;

namespace myoddweb.directorywatcher.utils.Helper
{
  internal class LoggerEvent : ILoggerEvent
  {
    /// <inheritdoc />
    public long Id { get; }

    /// <inheritdoc />
    public LogLevel LogLevel { get; }

    /// <inheritdoc />
    public string Message { get; }

    /// <summary>
    /// The constructor
    /// </summary>
    /// <param name="id"></param>
    /// <param name="logLevel"></param>
    /// <param name="message"></param>
    public LoggerEvent(long id, LogLevel logLevel, string message)
    {
      Id = id;
      LogLevel = logLevel;
      Message = message ?? throw new ArgumentNullException(nameof(message));
    }
  }
}
