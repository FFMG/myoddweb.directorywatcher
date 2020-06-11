using System;
using System.Collections.Generic;
using System.Linq;
using myoddweb.directorywatcher.interfaces;
using myoddweb.directorywatcher.utils.Helper;

namespace myoddweb.directorywatcher.utils
{
  internal abstract class WatcherManagerCommon : WatcherManager
  {
    #region Member variables
    /// <summary>
    /// The dictionary with all the events.
    /// </summary>
    private readonly Dictionary<long, IList<IEvent>> _idAndEvents = new Dictionary<long, IList<IEvent>>();

    /// <summary>
    /// Dictionary with the statistics
    /// </summary>
    private readonly Dictionary<long, IStatistics> _idStats = new Dictionary<long, IStatistics>();
    #endregion

    #region Protected Methods

    protected void StatisticsCallback(
      long elapsedTime,
      long numberOfEvents,
      long actualNumberOfMonitors
    )
    {
      lock (_idStats)
      {
      }
    }

    /// <summary>
    /// Function called at regular intervals when file events are detected.
    /// The intervals are controled in the 'start' function
    /// </summary>
    /// <param name="id"></param>
    /// <param name="isFile"></param>
    /// <param name="name"></param>
    /// <param name="oldName"></param>
    /// <param name="action"></param>
    /// <param name="error"></param>
    /// <param name="dateTimeUtc"></param>
    /// <returns></returns>
    protected int EventsCallback(
      long id,
      bool isFile,
      string name,
      string oldName,
      int action,
      int error,
      long dateTimeUtc)
    {
      lock (_idAndEvents)
      {
        if (!_idAndEvents.ContainsKey(id))
        {
          _idAndEvents[id] = new List<IEvent>();
        }
        _idAndEvents[id].Add(new Event(
          isFile,
          name,
          oldName,
          (EventAction)action,
          (interfaces.EventError)error,
          DateTime.FromFileTimeUtc(dateTimeUtc)
        ));
      }
      return 0;
    }
    #endregion

    #region Abstract methods
    public override bool GetStatistics(long id, out IStatistics statistics)
    {
      lock (_idStats)
      {
        // do we have that data at all?
        if (!_idStats.ContainsKey(id))
        {
          statistics = null;
          return false;
        }

        statistics = _idStats[id];
        _idAndEvents[id] = null;

        // the value coul be null
        return statistics != null;
      }
    }

    public override long GetEvents(long id, out IList<IEvent> events)
    {
      lock (_idAndEvents)
      {
        if (!_idAndEvents.ContainsKey(id))
        {
          events = new List<IEvent>();
          return 0;
        }

        events = _idAndEvents[id].Select(e => new Event(
          e.IsFile,
          e.Name,
          e.OldName,
          e.Action,
          e.Error,
          e.DateTimeUtc)).ToArray();
        _idAndEvents[id].Clear();
        return events.Count;
      }
    }

    public abstract override long Start(IRequest request);

    public abstract override bool Stop(long id);

    public abstract override bool Ready();

    #endregion
  }
}
