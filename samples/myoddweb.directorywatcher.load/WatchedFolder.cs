using System;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using myoddweb.directorywatcher.interfaces;
using myoddweb.directorywatcher.load.Output;

namespace myoddweb.directorywatcher.load
{
  internal class WatchedFolder
  {
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
    public IWatcher3 Watcher{ get; }

    /// <summary>
    /// The folder we are watching
    /// </summary>
    public DirectoryInfo Folder { get; }

    /// <summary>
    /// How we will be displaying the output information.
    /// </summary>
    private readonly IOutput _output;

    public WatchedFolder( IOutput output, DirectoryInfo directory, int howOftenStopWatch, IWatcher3 watcher)
    {
      _output = output ?? throw new ArgumentNullException(nameof(output));
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

    private Task OnRenamedAsync(IRenamedFileSystemEvent rfse, CancellationToken token)
    {
      _output.AddInformationMessage( rfse.DateTimeUtc, $"[{(rfse.IsFile ? "F" : "D")}][R]:{rfse.PreviousFileSystemInfo.FullName} > {rfse.FileSystemInfo.FullName}", token);
      return Task.CompletedTask;
    }

    private Task OnTouchedAsync(IFileSystemEvent fse, CancellationToken token)
    {
      _output.AddInformationMessage( fse.DateTimeUtc, $"[{(fse.IsFile ? "F" : "D")}][T]:{fse.FileSystemInfo.FullName}", token);
      return Task.CompletedTask;
    }

    private Task OnRemovedAsync(IFileSystemEvent fse, CancellationToken token)
    {
      _output.AddWarningMessage(fse.DateTimeUtc, $"[{(fse.IsFile ? "F" : "D")}][-]:{fse.FileSystemInfo.FullName}", token);
      return Task.CompletedTask;
    }

    private Task OnErrorAsync(IEventError ee, CancellationToken token)
    {
      _output.AddErrorMessage( ee.DateTimeUtc, $"[!]:{ee.Message}", token);
      return Task.CompletedTask;
    }

    private Task OnAddedAsync(IFileSystemEvent fse, CancellationToken token)
    {
      _output.AddInformationMessage( fse.DateTimeUtc, $"[{(fse.IsFile ? "F" : "D")}][+]:{fse.FileSystemInfo.FullName}", token);
      return Task.CompletedTask;
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
