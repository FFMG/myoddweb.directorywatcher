using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using System.Timers;
using myoddweb.directorywatcher.interfaces;
using myoddweb.directorywatcher.load.Output;
using Console = System.Console;
using Timer = System.Timers.Timer;

namespace myoddweb.directorywatcher.load
{
  /// <summary>
  /// A test that will add files and monitor for those changes.
  /// </summary>
  internal class Random : ILoad
  {
    /// <summary>
    /// The drive letters.
    /// </summary>
    private readonly IList<DirectoryInfo> _drives;

    /// <summary>
    /// The screen/file/noop output controller
    /// </summary>
    private readonly IOutput _output;

    /// <summary>
    /// How many iteration of changes we want to have
    /// </summary>
    private int _iterations;

    /// <summary>
    /// How we will be displaying the output information.
    /// </summary>
    private readonly IOutput _nonQuietOutput;
    
    /// <summary>
    /// The number of folders we are looking after
    /// </summary>
    private readonly List<WatchedFolder> _watchers = new List<WatchedFolder>();

    /// <summary>
    /// All the options
    /// </summary>
    private Options Options { get; }
    
    /// <summary>
    /// The current temp folder
    /// </summary>
    private string _tmpPath = null;

    private readonly List<string> _filesWaitingToAdd = new List<string>();
    private readonly List<string> _filesWaitingToRemove = new List<string>();

    /// <summary>
    /// The temp folder
    /// </summary>
    private string TempFolder
    {
      get
      {
        if (_tmpPath != null)
        {
          return _tmpPath;
        }
        _tmpPath = Path.Combine( Path.GetTempPath(), "myoddweb.directorywatcher");
        try
        {
          Directory.CreateDirectory(_tmpPath);
        }
        catch
        {
          // ignore
        }
        return _tmpPath;
      }
    }

    /// <summary>
    /// Display stas from time to time.
    /// </summary>
    private readonly Timer _displayUpdatesTimer = new Timer();

    public Random(Options options)
    {
      Options = options ?? throw new ArgumentNullException(nameof(options));

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

    public void Dispose()
    {
      Stop();
    }

    public void Start()
    {
      lock (_watchers)
      {
        // start them all
        foreach (var w in _watchers)
        {
          w.Watcher.OnLoggerAsync += OnLoggerAsync;
          w.Watcher.OnAddedAsync += OnAddedAsync;
          w.Watcher.OnRemovedAsync += OnRemovedAsync;
          w.Start();

          SpinWait.SpinUntil(() => w.Watcher.Ready());
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
          w.Watcher.OnLoggerAsync -= OnLoggerAsync;
          w.Watcher.OnAddedAsync -= OnAddedAsync;
          w.Watcher.OnRemovedAsync -= OnRemovedAsync;
          w.Stop();
        }
      }

      try
      {
        Directory.Delete(_tmpPath, true );
      }
      catch
      {
        // ignored
      }
    }

    private Task OnLoggerAsync(ILoggerEvent e, CancellationToken token)
    {
      _nonQuietOutput.AddMessage(e.Message, token);
      return Task.CompletedTask;
    }

    /// <summary>
    /// Create the watchers
    /// </summary>
    private void CreateWatchers()
    {
      // we need to create a watcher and add it to our list.
      var watcher = new Watcher();
      foreach (var drive in _drives)
      {
        _watchers.Add(new WatchedFolder(_output, drive, Options.Change, watcher));
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
        folders.Add(new DirectoryInfo(drive.Name));
      }
      return folders;
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
      // only add another watched folder if we still have iterations.
      --_iterations;
      if (_iterations <= 0)
      {
        Stop();

        lock (_filesWaitingToAdd)
        {
          foreach (var fullName in _filesWaitingToAdd)
          {
            _nonQuietOutput.AddErrorMessage(DateTime.UtcNow, $"Found added file that was not picked up! {fullName}", CancellationToken.None);
          }
        }

        lock (_filesWaitingToRemove)
        {
          foreach (var fullName in _filesWaitingToRemove)
          {
            _nonQuietOutput.AddErrorMessage(DateTime.UtcNow, $"Found removed file that was not picked up! {fullName}", CancellationToken.None);
          }
        }
        _nonQuietOutput.AddMessage("All done!", CancellationToken.None);
        return;
      }

      AddAFile();
      RemoveAFile();
    }

    private void AddAFile()
    {
      //  randomly add a file somewhere.
      var newFile = Path.Combine(TempFolder, $"{DateTime.UtcNow:HHmmssffff}.txt");
      lock (_filesWaitingToAdd)
      {
        if (_filesWaitingToAdd.Any())
        {
          _nonQuietOutput.AddErrorMessage(DateTime.UtcNow, $"Missing {_filesWaitingToAdd.Count} files that were not added!", CancellationToken.None);
        }
        _filesWaitingToAdd.Add(newFile);
      }

      // then actually create it.
      File.Create(newFile).Dispose();
    }

    private void RemoveAFile()
    {
      //  randomly add a file somewhere.
      var newFile = Path.Combine(TempFolder, $"{DateTime.UtcNow:HHmmssffff}.txt");
      lock (_filesWaitingToRemove)
      {
        if (_filesWaitingToRemove.Any())
        {
          _nonQuietOutput.AddErrorMessage(DateTime.UtcNow, $"Missing {_filesWaitingToRemove.Count} files that were not removed!", CancellationToken.None);
        }
        _filesWaitingToRemove.Add(newFile);
      }

      // then actually create it.
      File.Create(newFile).Dispose();

      // then remove it
      try
      {
        File.Delete(newFile);
      }
      catch
      {
        // ignore
      }
    }

    private Task OnRemovedAsync(IFileSystemEvent e, CancellationToken token)
    {
      lock (_filesWaitingToRemove)
      {
        if (_filesWaitingToRemove.Contains(e.FullName))
        {
          _filesWaitingToRemove.Remove(e.FullName);
          _nonQuietOutput.AddInformationMessage(DateTime.UtcNow, $"Found removed file: {e.FullName}", token);
        }
      }
      return Task.CompletedTask;
    }

    private Task OnAddedAsync(IFileSystemEvent e, CancellationToken token)
    {
      lock (_filesWaitingToAdd)
      {
        if (_filesWaitingToAdd.Contains(e.FullName))
        {
          _filesWaitingToAdd.Remove(e.FullName);
          try
          {
            File.Delete(e.FullName);
          }
          catch
          {
            // ignore
          }
          _nonQuietOutput.AddInformationMessage( DateTime.UtcNow, $"Found added file: {e.FullName}", token );
        }
      }
      return Task.CompletedTask;
    }
  }
}
