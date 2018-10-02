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
        var id1 = watch.Start("c:\\", true);
        //var id2 = watch.Start("h:\\", true);

        var exitEvent = new ManualResetEvent(false);
        Console.CancelKeyPress += delegate (object sender, ConsoleCancelEventArgs e)
        {
          e.Cancel = true;
          Console.WriteLine("Stop detected.");
          exitEvent.Set();

          // stop the monitor
          watch.Stop(id1);
          //watch.Stop(id2);
        };

        // do something
        // then wait for the user to press a key

        exitEvent.WaitOne();
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
  }
}
