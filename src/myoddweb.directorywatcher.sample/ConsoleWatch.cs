//This file is part of Myoddweb.Directorywatcher.
//
//    Myoddweb.Directorywatcher is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    Myoddweb.Directorywatcher is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Myoddweb.Directorywatcher.  If not, see<https://www.gnu.org/licenses/gpl-3.0.en.html>.
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
    private static readonly object _lock = new object();

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
      await AddMessage(ConsoleColor.Cyan, rfse.DateTimeUtc, $"[R]:{rfse.PreviousFileSystemInfo.FullName} > {rfse.FileSystemInfo.FullName}", token).ConfigureAwait(false);
    }

    private async Task OnRemovedAsync(IFileSystemEvent rfse, CancellationToken token)
    {
      await AddMessage(ConsoleColor.Yellow, rfse.DateTimeUtc, $"[-]:{rfse.FileSystemInfo.FullName}", token).ConfigureAwait(false);
    }

    private async Task OnAddedAsync(IFileSystemEvent fse, CancellationToken token)
    {
      await AddMessage(ConsoleColor.Green, fse.DateTimeUtc, $"[+]:{fse.FileSystemInfo.FullName}", token).ConfigureAwait(false);
    }

    private async Task OnErrorAsync(IEventError ee, CancellationToken token )
    {
      await AddMessage(ConsoleColor.Red, ee.DateTimeUtc, $"[!]:{ee.Message}", token).ConfigureAwait(false);
    }

    private async Task OnTouchedAsync(IFileSystemEvent rfse, CancellationToken token)
    {
      await AddMessage( ConsoleColor.Gray, rfse.DateTimeUtc, $"[T]:{rfse.FileSystemInfo.FullName}", token ).ConfigureAwait( false );
    }

    private async Task AddMessage(ConsoleColor color, DateTime dt, string message, CancellationToken token)
    {
      await Task.Run(() =>
      {
        lock (_lock)
        {
          Console.ForegroundColor = color;
          Console.WriteLine($"[{dt:HH:mm:ss.ffff}][T]:{message}");
          Console.ForegroundColor = _consoleColor;
        }
      }, token);
    }
  }
}
