namespace myoddweb.directorywatcher.interfaces
{
  public interface IWatcher1
  {
    /// <summary>
    /// The path we wish to monitor for changes
    /// </summary>
    /// <param name="path">The path we want to monitor.</param>
    /// <param name="recursive"></param>
    /// <returns>Unique Id used to release/stop monitoring</returns>
    long Monitor(string path, bool recursive);
  }
}
