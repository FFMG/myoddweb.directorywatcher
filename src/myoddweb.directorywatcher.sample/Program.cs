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
using myoddweb.directorywatcher.interfaces;
using Timer = System.Timers.Timer;

namespace myoddweb.directorywatcher.sample
{
  internal class Request : IRequest
  {
    public Request(string path, bool recursive)
    {
      Path = path;
      Recursive = recursive;
    }

    public string Path { get; }
    public bool Recursive { get; }
  }

  internal class Program
  {
    private static void Main()
    {
      try
      {
        Console.WriteLine("Press Ctrl+C to stop the monitors.");

        // start the monitor.
        IWatcher2 watch1 = new Watcher();
        watch1.Add(new Request("c:\\", true));
        watch1.Add(new Request("d:\\", true));
        watch1.Add(new Request("d:\\", true));
        
        var watch2 = new Watcher();
        watch2.Add( new Request("c:\\", true) );

        const double ms = 100;
        var timer = new Timer();
        timer.Elapsed += (obj, evnt) =>
        {
          watch1.GetEvents( out var events1 );
          foreach (var e in events1)
          {
            Console.WriteLine($"[{e.DateTimeUtc.Hour}:{e.DateTimeUtc.Minute}:{e.DateTimeUtc.Second}]:{e.Path}");
          }
          watch2.GetEvents(out var events2);
          foreach (var e in events2)
          {
            Console.WriteLine($"[{e.DateTimeUtc.Hour}:{e.DateTimeUtc.Minute}:{e.DateTimeUtc.Second}]:{e.Path}");
          }
          timer.Start();
        };
        timer.Interval = ms;
        timer.AutoReset = false;
        timer.Start();

        watch1.Start();
        watch2.Start();

        var exitEvent = new ManualResetEvent(false);
        Console.CancelKeyPress += delegate (object sender, ConsoleCancelEventArgs e)
        {
          e.Cancel = true;
          Console.WriteLine("Stop detected.");
          exitEvent.Set();
        };

        // do something
        // then wait for the user to press a key

        exitEvent.WaitOne();

        // stop everything.
        timer.Stop();
        watch1.Stop();
        watch2.Stop();
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
