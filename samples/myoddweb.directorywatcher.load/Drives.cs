using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Threading;
using System.Timers;
using myoddweb.directorywatcher.load.Output;
using Timer = System.Timers.Timer;

namespace myoddweb.directorywatcher.load
{
  internal class Drives : ILoad
  {
    /// <summary>
    /// The drive letters.
    /// </summary>
    private readonly IList<DirectoryInfo> _drives;

    /// <summary>
    /// How many iteration of changes we want to have
    /// </summary>
    private int _iterations;

    /// <summary>
    /// The number of folders we are looking after
    /// </summary>
    private readonly List<WatchedFolder> _watchers = new List<WatchedFolder>();

    /// <summary>
    /// All the options
    /// </summary>
    private Options Options { get; }

    /// <summary>
    /// The screen/file/noop output controller
    /// </summary>
    private readonly IOutput _output;

    /// <summary>
    /// How we will be displaying the output information.
    /// </summary>
    private readonly IOutput _nonQuietOutput;

    /// <summary>
    /// Display stas from time to time.
    /// </summary>
    private readonly Timer _displayUpdatesTimer = new Timer();

    public Drives(Options options )
    {
      Options = options ?? throw new ArgumentNullException( nameof(options));

      if (Options.Quiet)
      {
        // display nothing from the events
        _output = new Noop();
      }
      else
      {
        // display the messages out to the console.
        _output = new Output.Console();
      }
      _nonQuietOutput = new Output.Console();

      // get all the drives.
      _drives = GetDrives();

      // get the number of iterations
      _iterations = Options.Iterations;

      // create the folders we will be watching
      // we always include the root folders
      CreateWatchers();
    }

    public void Start()
    {
      lock (_watchers)
      {
        // start them all
        foreach (var w in _watchers)
        {
          w.Start();
        }
      }

      // start the timer
      StartTimers();
    }

    public void Stop()
    {
      StopTimers();

      // stop them all
      lock (_watchers)
      {
        foreach (var w in _watchers)
        {
          w.Stop();
        }
      }
    }

    /// <summary>
    /// Create the watchers
    /// </summary>
    private void CreateWatchers()
    {
      // we need to create a watcher and add it to our list.
      var watcher = new Watcher();
      foreach ( var drive in _drives )
      {
        _watchers.Add( new WatchedFolder(_output, drive, 0, watcher ));
      }
    }

    /// <summary>
    /// Get all the drive letters.
    /// </summary>
    /// <returns></returns>
    private static IList<DirectoryInfo> GetDrives()
    {
      var folders = new List<DirectoryInfo>();
      foreach (var drive in DriveInfo.GetDrives())
      {
        folders.Add( new DirectoryInfo( drive.Name) );
      }
      return folders;
    }

    /// <summary>
    /// Clean up
    /// </summary>
    public void Dispose()
    {
      Stop();
    }


    private void StopTimers()
    {
      _displayUpdatesTimer.Enabled = false;
      _displayUpdatesTimer.Elapsed -= OnDisplayUpdatesTimerEvent;
    }


    private void StartTimers()
    {
      StopTimers();

      // display stats
      _displayUpdatesTimer.Interval = 10 * 1000;  // every 10 seconds
      _displayUpdatesTimer.Elapsed += OnDisplayUpdatesTimerEvent;
      _displayUpdatesTimer.AutoReset = true;

      // start all.
      _displayUpdatesTimer.Enabled = true;
    }

    private void OnDisplayUpdatesTimerEvent(object sender, ElapsedEventArgs e)
    {
      // we need to display the nmuber if item
      var number = Process.GetCurrentProcess().Threads.Count;
      _nonQuietOutput.AddMessage($"Number of threads: {number}", CancellationToken.None);

      // stop and start again
      // this will ensure that we get rid of memory/treads and so on.
      Stop();

      // only add another watched folder if we still have iterations.
      --_iterations;
      if (_iterations <= 0)
      {
        _nonQuietOutput.AddMessage("All done!", CancellationToken.None);
        return;
      }

      Start();
    }
  }
}
