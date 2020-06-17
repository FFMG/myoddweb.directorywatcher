namespace myoddweb.directorywatcher.interfaces
{
  public interface IStatistics
  {
    /// <summary>
    /// The watched id 
    /// </summary>
    long Id { get; }

    /// <summary>
    /// The elapsed time since the last time we received a message
    /// </summary>
    double ElapsedTime { get; }

    /// <summary>
    /// The total number of events since the last stats call.
    /// </summary>
    long NumberOfEvents { get; }
  }
}
