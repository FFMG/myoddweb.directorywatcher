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
using System.Threading.Tasks;

namespace myoddweb.directorywatcher.sample
{
  internal class Program
  {
    private static void Main(string[] args)
    {
      // start the monitor.
      var watch = new Watcher();
      var id = watch.StartMonitor("c:\\", true );

      // do something
      Task.Delay( 30 * 1000 ).Wait();

      // stop the monitor
      watch.StopMonitor(id);
    }
  }
}
