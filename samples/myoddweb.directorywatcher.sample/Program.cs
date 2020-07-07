// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
using System;
using System.IO;
using System.Threading;
using System.Threading.Tasks;

namespace myoddweb.directorywatcher.sample
{
  internal class Program
  {
    private static async Task Main()
    {
      try
      {
        Console.WriteLine(Environment.Is64BitProcess ? "x64 version" : "x86 version");
        Console.WriteLine("Press Ctrl+C to stop the monitors.");

        // // start the monitor.
        // for (var i = 0; i < 10; ++i)
        // {
        //   await RunWatcherAsync().ConfigureAwait(false);
        //   Console.WriteLine("-------------------------------------------------------");
        // }
        // Console.WriteLine("All done.");
        // Console.WriteLine("Press Ctrl+C to stop to exit.");
        // WaitForCtrlC();

        //watch.Add(new Request("z:\\", true, new Rates(10000, 0)));

        // RunAndWait();

        await RunAndRenameFolderInRoot().ConfigureAwait( false );
      }
      catch (Exception ex)
      {
        WriteException(ex);
      }
    }

    private static async Task RunAndRenameFolderInRoot()
    {
      using (var watch = new Watcher())
      {
        var drvs = DriveInfo.GetDrives();
        foreach (var drv in drvs)
        {
          if (drv.DriveType == DriveType.Fixed)
          {
            watch.Add(new Request(drv.Name, true ));
          }
        }
        
        //const string path = "z:\\";
        //watch.Add(new Request(path, true, new Rates(50, 0)));

        watch.OnLoggerAsync += async (e, token) =>
        {
          await Task.Yield();
          Console.WriteLine($"[{DateTime.UtcNow:HH:mm:ss.ffff}]:{e.Message}");
        };

        // prepare the console watcher so we can output pretty messages.
        var _ = new ConsoleWatch(watch);

        // start watching
        watch.Start();

        var cts = new CancellationTokenSource();

        // start the renaming of folder
        const string path = "z:\\";
        var task = RenameFolders( $"{path}\\myoddweb.{Guid.NewGuid()}", 1000, cts.Token );

        Console.WriteLine("Press Ctrl+C to stop to exit.");
        WaitForCtrlC();

        // stop everything.
        watch.Stop();

        // only stop the task now
        // in case errors are thrown durring the shutdown itself
        cts.Cancel();

        // wait for the task
        await Task.WhenAll(task).ConfigureAwait(false);
      }
    }

    private static async Task RenameFolders( string folder, int sleep, CancellationToken token )
    {
      // create the folder if needbe
      if(!Directory.Exists(folder))
      { 
        //  if that throws then we might need to run as admin.
        Directory.CreateDirectory(folder);
      }

      var folderFrom = folder;
      var folderTo = $"{folder}.2";

      while (true)
      {
        if (token.IsCancellationRequested)
        {
          break;
        }

        Console.WriteLine( $"Swapping {folderFrom} to {folderTo}.");
        Directory.Move( folderFrom, folderTo );

        // swap the 2
        var t = folderFrom;
        folderFrom = folderTo;
        folderTo = t;

        await Task.Delay(sleep, token).ConfigureAwait(false);
      }

      // try and clear all
      Directory.Delete(folderFrom, true );
    }

    private static void RunAndWait()
    {
      using (var watch = new Watcher())
      {
        var drvs = System.IO.DriveInfo.GetDrives();
        foreach (var drv in drvs)
        {
          if (drv.DriveType == System.IO.DriveType.Fixed)
          {
            watch.Add(new Request(drv.Name, true, new Rates(50, 0)));
          }
        }

        // prepare the console watcher so we can output pretty messages.
        var _ = new ConsoleWatch(watch);

        // start watching
        watch.Start();

        Console.WriteLine("Press Ctrl+C to stop to exit.");
        WaitForCtrlC();

        // stop everything.
        watch.Stop();
      }
    }

    private static async Task RunWatcherAsync()
    {
      using (var watch = new Watcher())
      {
        var drvs = System.IO.DriveInfo.GetDrives();
        foreach (var drv in drvs)
        {
          if (drv.DriveType == System.IO.DriveType.Fixed)
          {
            watch.Add(new Request(drv.Name, true));
          }
        }

        // prepare the console watcher so we can output pretty messages.
        var _ = new ConsoleWatch(watch);

        // start watching
        watch.Start();

        // listen for the Ctrl+C 
        await Task.Delay(5000).ConfigureAwait(false);

        // stop everything.
        watch.Stop();
      }
    }

    private static void WriteException(Exception ex)
    {
      if (ex is AggregateException aggex)
      {
        WriteException(aggex.InnerException);
        foreach (var aggexInner in aggex.InnerExceptions)
        {
          WriteException(aggexInner);
        }
        return;
      }

      Console.WriteLine(ex.Message);
      while (ex.InnerException != null)
      {
        ex = ex.InnerException;
        Console.WriteLine(ex.Message);
      }
    }

    private static void WaitForCtrlC()
    {
      var exitEvent = new ManualResetEvent(false);
      Console.CancelKeyPress += delegate (object sender, ConsoleCancelEventArgs e)
      {
        e.Cancel = true;
        Console.WriteLine("Stop detected.");
        exitEvent.Set();
      };
      exitEvent.WaitOne();
    }
  }
}
