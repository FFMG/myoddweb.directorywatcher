namespace myoddweb.directorywatcher.interfaces
{
  public enum EventAction
  {
    /// <summary>
    /// General error
    /// </summary>
    Error,

    /// <summary>
    /// General memory error, (out of and so on).
    /// </summary>
    ErrorMemory,

    /// <summary>
    /// there was an overflow.
    /// </summary>
    ErrorOverflow,

    /// <summary>
    /// the monitoring was stopped somehow.
    /// </summary>
    ErrorAborted,

    /// <summary>
    /// Unable to even start the monitoring
    /// Is the path valid? Is the filename valid?
    /// </summary>
    ErrorCannotStart,

    /// <summary>
    /// We have an unknown file error.
    /// </summary>
    Unknown = 1000,
    Added,
    Removed,

    /// <summary>
    /// Small changed, timestamp, attribute etc...
    /// </summary>
    Touched,
    Renamed
  }
}