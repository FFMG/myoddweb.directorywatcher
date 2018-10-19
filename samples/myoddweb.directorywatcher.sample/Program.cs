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

namespace myoddweb.directorywatcher.sample
{
  internal class Program
  {
    private static void Main()
    {
      try
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

        // prepare the console watcher so we can output pretty messages.
        var cw = new ConsoleWatch(watch);

        // start watching
        watch.Start();

        // listen for the Ctrl+C 
        WaitForCtrlC();

        // stop everything.
        watch.Stop();
      }
      catch (Exception ex)
      {
        Console.WriteLine(ex.Message);
        while (ex.InnerException != null)
        {
          ex = ex.InnerException;
          Console.WriteLine(ex.Message);
        }
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
