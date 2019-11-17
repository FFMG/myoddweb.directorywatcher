using myoddweb.directorywatcher;
using myoddweb.directorywatcher.interfaces;
using System;
using System.Threading;
using System.Threading.Tasks;

namespace nonblocking
{
  internal class Program
  {
    private static readonly IWatcher3 Watcher = new Watcher();

    private static void Main()
    {
      Console.WriteLine( Environment.Is64BitProcess ? "64-bit process" : "32-bit process" );
      Console.WriteLine( "Press Ctrl+C to stop the monitors." ); 

      // start the monitor.
      var drvs = System.IO.DriveInfo.GetDrives();
      foreach (var drv in drvs)
      {
        if (drv.DriveType == System.IO.DriveType.Fixed)
        {
          Watcher.Add(new Request(drv.Name, true));
        }
      }

      Watcher.OnTouchedAsync += OnTouchedAsync;

      Watcher.Start();

      WaitForCtrlC();

      Watcher.Stop();
    }

    private static Task OnTouchedAsync(IFileSystemEvent fse, CancellationToken token)
    {
      Console.WriteLine($"[{fse.DateTimeUtc:HH:mm:ss.ffff}]:[{fse.FileSystemInfo.FullName}" );

      var rng = new Random();
      if( rng.Next(100) == 0 )
      {
        Console.WriteLine("Calling STOP in loop - Start!");
        Watcher.Stop();
        Console.WriteLine("Calling STOP in loop - End!");
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
