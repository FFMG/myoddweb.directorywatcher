using System;

namespace myoddweb.directorywatcher.load
{
  public interface ILoad : IDisposable
  {
    /// <summary>
    /// Start the test runnint
    /// </summary>
    void Start();

    /// <summary>
    /// Top the test running
    /// </summary>
    void Stop();
  }
}
