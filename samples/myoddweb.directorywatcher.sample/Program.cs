// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
using System;
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

        RunAndWait();
      }
      catch (Exception ex)
      {
        WriteException(ex);
      }
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
        //watch.Add(new Request("z:\\", true, new Rates(10000, 0)));

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
