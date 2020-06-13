namespace myoddweb.directorywatcher.interfaces
{
  /// <summary>
  /// A logger event
  /// </summary>
  public interface ILoggerEvent
  {
    /// <summary>
    /// The id of the request
    /// </summary>
    long Id { get; }

    /// <summary>
    /// The message log level...
    /// </summary>
    LogLevel LogLevel { get; }

    /// <summary>
    /// The actual message
    /// </summary>
    string Message { get; }
  }
}