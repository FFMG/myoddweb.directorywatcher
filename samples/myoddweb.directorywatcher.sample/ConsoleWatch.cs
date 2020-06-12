// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
using System;
using System.Threading;
using System.Threading.Tasks;
using myoddweb.directorywatcher.interfaces;

namespace myoddweb.directorywatcher.sample
{
  internal class ConsoleWatch
  {
    /// <summary>
    /// The original console color
    /// </summary>
    private readonly ConsoleColor _consoleColor;

    /// <summary>
    /// We need a static lock so it is shared by all.
    /// </summary>
    private static readonly object Lock = new object();

    public ConsoleWatch(IWatcher2 watch)
    {
      _consoleColor = Console.ForegroundColor;

      // technically we should dispose of those at the end ...
      // but I live on the wild side.
      watch.OnErrorAsync += OnErrorAsync;
      watch.OnAddedAsync += OnAddedAsync;
      watch.OnRemovedAsync += OnRemovedAsync;
      watch.OnRenamedAsync += OnRenamedAsync;
      watch.OnTouchedAsync += OnTouchedAsync;
      watch.OnStatisticsAsync += OnStatisticsAsync;
      watch.OnLoggerAsync += OnOnLoggerAsync;
    }

    private async Task OnOnLoggerAsync(ILoggerEvent e, CancellationToken token)
    {
      await AddMessage(ConsoleColor.DarkBlue, DateTime.UtcNow, $"Id:{e.Id}\n" + e.Message, token).ConfigureAwait(false);
    }

    private async Task OnStatisticsAsync(IStatistics e, CancellationToken token)
    {
      await AddMessage(ConsoleColor.DarkYellow, DateTime.UtcNow, 
        $"Id:{e.Id}\n"+
        $"Number Of Events: {e.NumberOfEvents}\n"+
        $"Elapsed Time: {e.ElapsedTime}", 
        token).ConfigureAwait(false);
    }

    private async Task OnRenamedAsync(IRenamedFileSystemEvent rfse, CancellationToken token)
    {
      await AddMessage(ConsoleColor.Cyan, rfse.DateTimeUtc, $"[{(rfse.IsFile ? "F" : "D")}][R]:{rfse.PreviousFileSystemInfo.FullName} > {rfse.FileSystemInfo.FullName}", token).ConfigureAwait(false);
    }

    private async Task OnRemovedAsync(IFileSystemEvent fse, CancellationToken token)
    {
      await AddMessage(ConsoleColor.Yellow, fse.DateTimeUtc, $"[{(fse.IsFile ? "F" : "D")}][-]:{fse.FileSystemInfo.FullName}", token).ConfigureAwait(false);
    }

    private async Task OnAddedAsync(IFileSystemEvent fse, CancellationToken token)
    {
      await AddMessage(ConsoleColor.Green, fse.DateTimeUtc, $"[{(fse.IsFile ? "F" : "D")}][+]:{fse.FileSystemInfo.FullName}", token).ConfigureAwait(false);
    }

    private async Task OnErrorAsync(IEventError ee, CancellationToken token )
    {
      await AddMessage(ConsoleColor.Red, ee.DateTimeUtc, $"[!]:{ee.Message}", token).ConfigureAwait(false);
    }

    private async Task OnTouchedAsync(IFileSystemEvent fse, CancellationToken token)
    {
      await AddMessage( ConsoleColor.Gray, fse.DateTimeUtc, $"[{(fse.IsFile?"F":"D")}][T]:{fse.FileSystemInfo.FullName}", token ).ConfigureAwait( false );
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
            Console.WriteLine( e.Message );
          }
          finally
          {
            Console.ForegroundColor = _consoleColor;
          }
        }
      }, token);
    }
  }
}
