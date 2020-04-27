using System;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using myoddweb.directorywatcher.interfaces;

namespace myoddweb.directorywatcher.load
{
  internal class WatchedFolder
  {
    /// <summary>
    /// The original console color
    /// </summary>
    private readonly ConsoleColor _consoleColor;

    /// <summary>
    /// We need a static lock so it is shared by all.
    /// </summary>
    private static readonly object Lock = new object();

    /// <summary>
    /// How often we want to stop/start watching.
    /// </summary>
    private readonly int _howOftenStopWatch;

    /// <summary>
    /// If we started, the id.
    /// </summary>
    private long _id;

    /// <summary>
    /// The watcher we will be using
    /// </summary>
    public IWatcher2 Watcher{ get; }

    /// <summary>
    /// The folder we are watching
    /// </summary>
    public DirectoryInfo Folder { get; }

    public WatchedFolder(DirectoryInfo directory, int howOftenStopWatch, IWatcher2 watcher)
    {
      _consoleColor = Console.ForegroundColor;

      Folder = directory ?? throw new ArgumentNullException(nameof(directory));
      _howOftenStopWatch = howOftenStopWatch;
      Watcher = watcher ?? throw new ArgumentNullException(nameof(watcher));

      AddWatch();
    }

    private void AddWatch()
    {
      lock (Lock)
      {
        // start watching it
        Watcher.OnAddedAsync += OnAddedAsync;
        Watcher.OnErrorAsync += OnErrorAsync;
        Watcher.OnRemovedAsync += OnRemovedAsync;
        Watcher.OnRenamedAsync += OnRenamedAsync;
        Watcher.OnTouchedAsync += OnTouchedAsync;
      }
    }

    private void RemoveWatch()
    {
      lock (Lock)
      {
        // start watching it
        Watcher.OnAddedAsync -= OnAddedAsync;
        Watcher.OnErrorAsync -= OnErrorAsync;
        Watcher.OnRemovedAsync -= OnRemovedAsync;
        Watcher.OnRenamedAsync -= OnRenamedAsync;
        Watcher.OnTouchedAsync -= OnTouchedAsync;
      }
    }

    private async Task OnRenamedAsync(IRenamedFileSystemEvent rfse, CancellationToken token)
    {
      await AddMessage(ConsoleColor.Cyan, rfse.DateTimeUtc, $"[{(rfse.IsFile ? "F" : "D")}][R]:{rfse.PreviousFileSystemInfo.FullName} > {rfse.FileSystemInfo.FullName}", token).ConfigureAwait(false);
    }

    private async Task OnTouchedAsync(IFileSystemEvent fse, CancellationToken token)
    {
      await AddMessage(ConsoleColor.Gray, fse.DateTimeUtc, $"[{(fse.IsFile ? "F" : "D")}][T]:{fse.FileSystemInfo.FullName}", token).ConfigureAwait(false);
    }

    private async Task OnRemovedAsync(IFileSystemEvent fse, CancellationToken token)
    {
      await AddMessage(ConsoleColor.Yellow, fse.DateTimeUtc, $"[{(fse.IsFile ? "F" : "D")}][-]:{fse.FileSystemInfo.FullName}", token).ConfigureAwait(false);
    }

    private async Task OnErrorAsync(IEventError ee, CancellationToken token)
    {
      await AddMessage(ConsoleColor.Red, ee.DateTimeUtc, $"[!]:{ee.Message}", token).ConfigureAwait(false);
    }

    private async Task OnAddedAsync(IFileSystemEvent fse, CancellationToken token)
    {
      await AddMessage(ConsoleColor.Green, fse.DateTimeUtc, $"[{(fse.IsFile ? "F" : "D")}][+]:{fse.FileSystemInfo.FullName}", token).ConfigureAwait(false);
    }

    private async Task AddMessage(ConsoleColor color, DateTime dt, string message, CancellationToken token)
    {
      await Task.Run(() =>
      {
        lock (Lock)
        {
          try
          {
            Console.ForegroundColor = color;
            Console.WriteLine($"[{dt:HH:mm:ss.ffff}]:{message}");
          }
          catch (Exception e)
          {
            Console.ForegroundColor = ConsoleColor.Red;
            Console.WriteLine(e.Message);
          }
          finally
          {
            Console.ForegroundColor = _consoleColor;
          }
        }
      }, token);
    }

    public void Start()
    {
      Stop();
      lock (Lock)
      {
        _id = Watcher.Start(new Request(Folder.FullName, true));
        AddWatch();
      }
    }

    public void Stop()
    {
      if (_id == 0)
      {
        return;
      }

      lock (Lock)
      {
        if (_id == 0)
        {
          return;
        }

        RemoveWatch();
        Watcher.Stop(_id);
        _id = 0;
      }
    }
  }
}
