using System;
using myoddweb.commandlineparser.Interfaces;

namespace myoddweb.directorywatcher.load
{
  internal class Options
  {
    public int Iterations {get;}

    public int Folders { get; }

    public int Change { get; }

    public bool Unique { get; }

    /// <summary>
    /// If we want to run a random check test 
    /// </summary>
    public bool Random { get; }

    /// <summary>
    /// If we want to test all drives.
    /// </summary>
    public bool Drives { get; }

    /// <summary>
    /// If we want to display event messages or not.
    /// </summary>
    public bool Quiet { get; }

    public Options(ICommandlineParser arguments )
    {
      // the number of iterations we want to run
      // this is how often we want to 'stop/start' watching a folder.
      Iterations = arguments.Get<int>("i");
      if (Iterations <= 0)
      {
        throw new ArgumentException("The number of iterations cannot be -ve or zero", nameof(Iterations));
      }

      // the number of folders we want to watch.
      Folders = arguments.Get<int>("f");
      if (Folders <= 0)
      {
        throw new ArgumentException("The number of folders cannot be -ve or zero", nameof(Folders));
      }

      // how often we want to change folder in seconds.
      Change = arguments.Get<int>("c");

      // unique watcher or share the same one between watchers.
      Unique = arguments.Get<bool>("u");

      // display events messages or not?
      Quiet = arguments.Get<bool>("q");

      // if we are testing the drive letters only.
      Drives = arguments.Get<bool>("d");

      // are we testing for random events?
      Random = arguments.Get<bool>("r");
    }
  }
}
