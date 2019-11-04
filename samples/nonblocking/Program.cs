using myoddweb.directorywatcher;
using myoddweb.directorywatcher.interfaces;
using System;
using System.Threading;
using System.Threading.Tasks;

namespace nonblocking
{
  class Program
  {
    static void Main(string[] args)
    {
      Console.WriteLine("Press Ctrl+C to stop the monitors.");

      // start the monitor.
      var watch = new Watcher();
      var drvs = System.IO.DriveInfo.GetDrives();
      foreach (var drv in drvs)
      {
        if (drv.DriveType == System.IO.DriveType.Fixed)
        {
          watch.Add(new Request(drv.Name, true));
        }
      }

      watch.OnTouchedAsync += OnTouchedAsync;

      watch.Start();

      WaitForCtrlC();

      watch.Stop();
    }

    private static Task OnTouchedAsync(IFileSystemEvent e, CancellationToken token)
    {
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
