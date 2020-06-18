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
using Timer = System.Timers.Timer;

namespace myoddweb.directorywatcher.load
{
  internal class Load : ILoad
  {
    /// <summary>
    /// How many iteration of changes we want to have
    /// </summary>
    private int _iterations;
    
    /// <summary>
    /// The drive letters.
    /// </summary>
    private readonly IList<DirectoryInfo> _drives;

    /// <summary>
    /// All the folders we can watch
    /// </summary>
    private readonly IList<DirectoryInfo> _folders;

    /// <summary>
    /// The number of folders we are looking after
    /// </summary>
    private readonly List<WatchedFolder> _watchers = new List<WatchedFolder>();

    /// <summary>
    /// This is the timer to add new folders
    /// </summary>
    private readonly Timer _addWatchedFolderTimer = new Timer();

    /// <summary>
    /// This is the folder to remove watched folders.
    /// </summary>
    private readonly Timer _removeWatchedFolderTimer = new Timer();

    /// <summary>
    /// Display stas from time to time.
    /// </summary>
    private readonly Timer _displayUpdatesTimer = new Timer();

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

    public Load(Options options )
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

      _drives = GetDrives();

      _iterations = Options.Iterations;

      _folders = GetFolders();

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
          w.Watcher.OnLoggerAsync -= OnLoggerAsync;
          w.Stop();
        }
      }
    }

    private void StartTimers()
    {
      StopTimers();

      // the add folder
      _addWatchedFolderTimer.Interval = Options.Change * 1000;
      _addWatchedFolderTimer.Elapsed += OnAddFolderTimerEvent;
      _addWatchedFolderTimer.AutoReset = true;

      // the remove folder
      _removeWatchedFolderTimer.Interval = Options.Change * 1000;
      _removeWatchedFolderTimer.Elapsed += OnRemoveFolderTimerEvent;
      _removeWatchedFolderTimer.AutoReset = true;

      // display stats
      _displayUpdatesTimer.Interval = 10 * 1000;  // every 10 seconds
      _displayUpdatesTimer.Elapsed += OnDisplayUpdatesTimerEvent;
      _displayUpdatesTimer.AutoReset = true;

        // start all.
      _addWatchedFolderTimer.Enabled = true;
      _removeWatchedFolderTimer.Enabled = true;
      _displayUpdatesTimer.Enabled = true;
    }

    private void OnAddFolderTimerEvent(object sender, ElapsedEventArgs e)
    {
      // only add another watched folder if we still have iterations.
      --_iterations;
      if (_iterations <= 0)
      {
        Stop();
        _nonQuietOutput.AddMessage("All done!", CancellationToken.None);
        return;
      }

      // add another watcher
      var newWatchedFolder = AddWatchedFolder();
      newWatchedFolder.Start();
    }

    private void OnDisplayUpdatesTimerEvent(object sender, ElapsedEventArgs e)
    {
      // we need to display the nmuber if item
      var number = Process.GetCurrentProcess().Threads.Count;
      _nonQuietOutput.AddMessage($"Number of threads: {number}", CancellationToken.None);
    }

    private void OnRemoveFolderTimerEvent(object sender, ElapsedEventArgs e)
    {
      // remove a watched folder
      RemoveWatchedFolder();
    }

    /// <summary>
    /// Try and remove one watched folder
    /// </summary>
    private void RemoveWatchedFolder()
    {
      // get one of the watcher
      var watchedFolder = GetRandomWatchedFolder();
      if (null == watchedFolder)
      {
        return;
      }

      // stop the event
      watchedFolder.Stop();
      lock (_watchers)
      {
        _watchers.Remove(watchedFolder);
      }

      // add a note
      _nonQuietOutput.AddMessage($"Stopped Watching folder: {watchedFolder.Folder.Name}", CancellationToken.None);
    }

    private void StopTimers()
    {
      _addWatchedFolderTimer.Enabled = false;
      _addWatchedFolderTimer.Elapsed -= OnAddFolderTimerEvent;

      _removeWatchedFolderTimer.Enabled = false;
      _removeWatchedFolderTimer.Elapsed -= OnRemoveFolderTimerEvent;

      _displayUpdatesTimer.Enabled = false;
      _displayUpdatesTimer.Elapsed -= OnDisplayUpdatesTimerEvent;
    }

    /// <summary>
    /// Get all the folders/sub-folders up to a max of _numFolders
    /// </summary>
    /// <returns></returns>
    private IList<DirectoryInfo> GetFolders()
    {
      // get all the folders
      var folders = new List<DirectoryInfo>();

      // always use the drive letters
      folders.AddRange(GetDrives());

      foreach (var drive in _drives)
      {
        folders.Add(drive);
        folders.AddRange(GetSubfolders(drive));
      }
      return folders;
    }

    /// <summary>
    /// Create the watchers
    /// </summary>
    private void CreateWatchers()
    {
      // we need to create a watcher and add it to our list.
      for (var i = 0; i < Options.Folders; ++i)
      {
        AddWatchedFolder();
      }
    }

    private WatchedFolder AddWatchedFolder()
    {
      IWatcher3 watcher;
      if ( !Options.Unique )
      {
        watcher = new Watcher();
        watcher.OnLoggerAsync += OnLoggerAsync;
      }
      else
      {
        lock (_watchers)
        {
          watcher = _watchers.FirstOrDefault()?.Watcher;
          if (null == watcher)
          {
            watcher = new Watcher();
            watcher.OnLoggerAsync += OnLoggerAsync;
          }
        }
      }

      var watchedFolder = new WatchedFolder(
        _output,
        GetRandomFolder(),
        Options.Change,
        watcher
      );
      lock (_watchers)
      {
        _watchers.Add(watchedFolder );
      }

      _nonQuietOutput.AddMessage($"Adding another watched folder: {watchedFolder.Folder}", CancellationToken.None);

      return watchedFolder;
    }

    private Task OnLoggerAsync(ILoggerEvent e, CancellationToken token)
    {
      _nonQuietOutput.AddMessage(e.Message, token);
      return Task.CompletedTask;
    }

    /// <summary>
    /// Get a random folder from our list of folders.
    /// </summary>
    /// <returns></returns>
    private DirectoryInfo GetRandomFolder( )
    {
      return _folders.OrderBy(x => Guid.NewGuid()).FirstOrDefault();
    }

    /// <summary>
    /// Get a random watcher.
    /// </summary>
    /// <returns></returns>
    private WatchedFolder GetRandomWatchedFolder()
    {
      lock (_watchers)
      {
        return _watchers.OrderBy(x => Guid.NewGuid()).FirstOrDefault();
      }
    }

    /// <summary>
    /// Get subfolders from a root folder.
    /// </summary>
    /// <param name="root">The root folder</param>
    /// <returns></returns>
    private IEnumerable<DirectoryInfo> GetSubfolders(DirectoryInfo root )
    {
      try
      {
        var folders = new List<DirectoryInfo>();
        var directories = root.GetDirectories();

        foreach (var folder in directories)
        {
          folders.Add( folder );
          folders.AddRange( GetSubfolders( folder ));
          if (folders.Count > Options.Folders)
          {
            break;
          }
        }

        return folders;
      }
      catch( UnauthorizedAccessException)
      {
        // probably not allowed
        // ignore this error
        return new List<DirectoryInfo>();
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
      _addWatchedFolderTimer?.Dispose();
      _removeWatchedFolderTimer?.Dispose();
      _displayUpdatesTimer?.Dispose();
    }
  }
}
