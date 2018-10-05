using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using myoddweb.directorywatcher.interfaces;

namespace myoddweb.directorywatcher.sample
{
  internal class ConsoleWatch
  {
    private readonly ConsoleColor _consoleColor;

    public ConsoleWatch(IWatcher2 watch)
    {
      _consoleColor = Console.ForegroundColor;

      watch.OnErrorAsync += OnErrorAsync;
      watch.OnAddedAsync += OnAddedAsync;
      watch.OnRemovedAsync += OnRemovedAsync;
      watch.OnRenamedAsync += OnRenamedAsync;
      watch.OnTouchedAsync += OnTouchedAsync;
    }

    private async Task OnRenamedAsync(IRenamedFileSystemEvent rfse, CancellationToken token)
    {
      await Task.Run(() =>
      {
        Console.ForegroundColor = ConsoleColor.Blue;
        Console.WriteLine($"[{rfse.DateTimeUtc.Hour}:{rfse.DateTimeUtc.Minute}:{rfse.DateTimeUtc.Second}]:{rfse.FileSystemInfo}");
        Console.ForegroundColor = _consoleColor;
      }, token);
    }

    private async Task OnRemovedAsync(IFileSystemEvent rfse, CancellationToken token)
    {
      await Task.Run(() =>
      {
        Console.ForegroundColor = ConsoleColor.Yellow;
        Console.WriteLine( $"[{rfse.DateTimeUtc.Hour}:{rfse.DateTimeUtc.Minute}:{rfse.DateTimeUtc.Second}]:{rfse.FileSystemInfo}");
        Console.ForegroundColor = _consoleColor;
      }, token);
    }

    private async Task OnAddedAsync(IFileSystemEvent fse, CancellationToken token)
    {
      await Task.Run(() =>
      {
        Console.ForegroundColor = ConsoleColor.Green;
        Console.WriteLine( $"[{fse.DateTimeUtc.Hour}:{fse.DateTimeUtc.Minute}:{fse.DateTimeUtc.Second}]:{fse.FileSystemInfo}");
        Console.ForegroundColor = _consoleColor;
      }, token);
    }

    private async Task OnErrorAsync(IEventError ee, CancellationToken token )
    {
      await Task.Run(() =>
      {
        Console.ForegroundColor = ConsoleColor.Red;
        Console.WriteLine($"[{ee.DateTimeUtc.Hour}:{ee.DateTimeUtc.Minute}:{ee.DateTimeUtc.Second}]:{ee.Message}");
        Console.ForegroundColor = _consoleColor;
      }, token);
    }

    private async Task OnTouchedAsync(IFileSystemEvent rfse, CancellationToken token)
    {
      await Task.Run(() =>
      {
        Console.ForegroundColor = ConsoleColor.Gray;
        Console.WriteLine($"[{rfse.DateTimeUtc.Hour}:{rfse.DateTimeUtc.Minute}:{rfse.DateTimeUtc.Second}]:{rfse.FileSystemInfo}");
        Console.ForegroundColor = _consoleColor;
      }, token);
    }
  }
}
