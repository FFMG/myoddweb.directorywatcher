using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace myoddweb.directorywatcher.interfaces
{
  /// <summary>
  /// The various rates of refresh
  /// </summary>
  public interface IRates
  {
    /// <summary>
    /// How often we would like the libraries posted
    /// If the value is -1 then the statistics are not published.
    /// </summary>
    long StatisticsMilliseconds { get; }

    /// <summary>
    /// How quickly we want to publish events
    /// If this value is -1 then events are not published, (but still recorded)
    /// </summary>
    long EventsMilliseconds { get; }
  }
}
