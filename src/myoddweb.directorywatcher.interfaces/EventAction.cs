namespace myoddweb.directorywatcher.interfaces
{
  public enum EventAction
  {
    /// <summary>
    /// General error
    /// </summary>
    Error = 0,

    /// <summary>
    /// General memory error, (out of and so on).
    /// </summary>
    ErrorMemory = 1,

    /// <summary>
    /// there was an overflow.
    /// </summary>
    ErrorOverflow = 2,

    /// <summary>
    /// the monitoring was stopped somehow.
    /// </summary>
    ErrorAborted = 3,

    /// <summary>
    /// Unable to even start the monitoring
    /// Is the path valid? Is the filename valid?
    /// </summary>
    ErrorCannotStart = 4,

    /// <summary>
    /// Cannot access the file/folder
    /// </summary>
    ErrorAccess = 5,

    /// <summary>
    /// We have an unknown file error.
    /// </summary>
    Unknown = 1000,
    Added = 1001,
    Removed = 1002,

    /// <summary>
    /// Small changed, timestamp, attribute etc...
    /// </summary>
    Touched = 1003,
    Renamed = 1004
  }
}