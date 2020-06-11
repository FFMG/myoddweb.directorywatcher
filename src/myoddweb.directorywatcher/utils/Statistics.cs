using myoddweb.directorywatcher.interfaces;

namespace myoddweb.directorywatcher.utils
{
  internal class Statistics : IStatistics
  {
    /// <inheritdoc />
    public long Id { get; }

    /// <inheritdoc />
    public double ElapsedTime { get; }

    /// <inheritdoc />
    public long NumberOfEvents { get; }

    public Statistics( long id, double elapsedTime, long numberOfEvents)
    {
      Id = id;
      ElapsedTime = elapsedTime;
      NumberOfEvents = numberOfEvents;
    }
  }
}
