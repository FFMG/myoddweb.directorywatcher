using myoddweb.directorywatcher;
using myoddweb.directorywatcher.interfaces;
using System;
using System.Threading;
using System.Threading.Tasks;

namespace nonblocking
{
  class Program
  {
    private static Watcher _watcher = new Watcher();
    static void Main(string[] args)
    {
      Console.WriteLine("Press Ctrl+C to stop the monitors.");

      // start the monitor.
      var drvs = System.IO.DriveInfo.GetDrives();
      foreach (var drv in drvs)
      {
        if (drv.DriveType == System.IO.DriveType.Fixed)
        {
          _watcher.Add(new Request(drv.Name, true));
        }
      }

      _watcher.OnTouchedAsync += OnTouchedAsync;

      _watcher.Start();

      WaitForCtrlC();

      _watcher.Stop();
    }

    private static Task OnTouchedAsync(IFileSystemEvent fse, CancellationToken token)
    {
      Console.WriteLine($"[{fse.DateTimeUtc:HH:mm:ss.ffff}]:[{fse.FileSystemInfo.FullName}" );

      var rng = new Random();
      if( rng.Next(10) == 0 )
      {
        Console.WriteLine("Calling STOP in loop!");
        _watcher.Stop();
      }
      return Task.CompletedTask;
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
