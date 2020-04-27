using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Timers;
using myoddweb.directorywatcher.interfaces;

namespace myoddweb.directorywatcher.load
{
  internal class Load : IDisposable
  {
    /// <summary>
    /// How many iteration of changes we want to have
    /// </summary>
    private int _iterations;

    /// <summary>
    /// How many folder we want to watch
    /// </summary>
    private readonly int _numFolders;

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
    /// How often we want to change
    /// </summary>
    private readonly int _change;

    /// <summary>
    /// Our timer
    /// </summary>
    private readonly Timer _timer = new Timer();

    /// <summary>
    /// If the watcher is unique or not
    /// </summary>
    private readonly bool _unique;

    public Load(int iterations, int numFolders, int change, bool unique)
    {
      if (iterations <= 0)
      {
        throw new ArgumentException("The number of iterations cannot be -ve or zero", nameof(iterations));
      }
      _iterations = iterations;

      if (numFolders <= 0)
      {
        throw new ArgumentException("The number of folders cannot be -ve or zero", nameof(numFolders));
      }
      _numFolders = numFolders;
      _change = change;
      _unique = unique;

      _drives = GetDrives();

      _folders = GetFolders();

      // create the folders we will be watching
      // we always include the root folders
      CreateWatchers();
    }

    public void Start()
    {
      // start them all
      foreach (var w in _watchers)
      {
        w.Start();
      }

      // start the timer
      StartTimer();
    }

    public void Stop()
    {
      StopTimer();

      // stop them all
      foreach (var w in _watchers)
      {
        w.Stop();
      }
    }

    private void StartTimer()
    {
      StopTimer();
      _timer.Interval = _change * 1000;
      _timer.Elapsed += OnTimerEvent;
      _timer.AutoReset = true;
      _timer.Enabled = true;
    }

    private void OnTimerEvent(object sender, ElapsedEventArgs e)
    {
      // get one of the watcher
      var watchedFolder = GetRandomWatchedFolder();
      if (null == watchedFolder)
      {
        return;
      }

      --_iterations;
      if (_iterations <= 0)
      {
        Stop();
        Console.WriteLine("All done!");
        return;
      }

      // stop the event
      watchedFolder.Stop();
      _watchers.Remove(watchedFolder);

      // add another watcher
      var newWatchedFolder = AddWatchedFolder();
      newWatchedFolder.Start();
    }

    private void StopTimer()
    {
      _timer.Enabled = false;
      _timer.Elapsed -= OnTimerEvent;
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
      for (var i = 0; i < _numFolders; ++i)
      {
        AddWatchedFolder();
      }
    }

    private WatchedFolder AddWatchedFolder()
    {
      IWatcher2 watcher;
      if ( !_unique )
      {
        watcher = new Watcher();
      }
      else
      {
        watcher = _watchers.FirstOrDefault()?.Watcher ?? new Watcher();
      }

      var watchedFolder = new WatchedFolder(
        GetRandomFolder(),
        _change,
        watcher
      );
      _watchers.Add(watchedFolder );

      Console.WriteLine( $"Adding another watched folder: {watchedFolder.Folder}");

      return watchedFolder;
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
      return _watchers.OrderBy(x => Guid.NewGuid()).FirstOrDefault();
    }

    /// <summary>
    /// Get subfolders from a root folder.
    /// </summary>
    /// <param name="root">The root folder</param>
    /// <returns></returns>
    private IList<DirectoryInfo> GetSubfolders(DirectoryInfo root )
    {
      try
      {
        var folders = new List<DirectoryInfo>();
        var directories = root.GetDirectories();

        foreach (var folder in directories)
        {
          folders.Add( folder );
          folders.AddRange( GetSubfolders( folder ));
          if (folders.Count > _numFolders)
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
      _timer?.Dispose();
    }
  }
}
