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
    /// The message type, error, warning, info...
    /// </summary>
    int Type { get; }

    /// <summary>
    /// The actual message
    /// </summary>
    string Message { get; }
  }
}