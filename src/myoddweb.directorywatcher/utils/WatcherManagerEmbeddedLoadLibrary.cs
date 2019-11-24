using myoddweb.directorywatcher.interfaces;
using myoddweb.directorywatcher.utils.Helper;
using System;
using System.Collections.Generic;
using System.Diagnostics.Contracts;
using System.Linq;

namespace myoddweb.directorywatcher.utils
{
  internal class WatcherManagerEmbeddedLoadLibrary : WatcherManager
  {
    #region Member variables
    /// <summary>
    /// The dictionary with all the events.
    /// </summary>
    private readonly Dictionary<long, IList<IEvent>> _idAndEvents = new Dictionary<long, IList<IEvent>>();

    /// <summary>
    /// The Native dll helper
    /// </summary>    
    private readonly WatcherManagerNativeLibrary _helper;
    #endregion

    public WatcherManagerEmbeddedLoadLibrary()
    {
      // Create helper we will throw if the file does not exist.
      _helper = new WatcherManagerNativeLibrary(GetFromEmbedded(), Callback);
    }

    #region Private Methods
    private string GetFromEmbedded()
    {
      return Environment.Is64BitProcess ? GetFromEmbeddedx64() : GetFromEmbeddedx86();
    }

    private string GetFromEmbeddedx86()
    {
      Contract.Assert(!Environment.Is64BitProcess);
      return CreateResourceFile("x86.directorywatcher.win", "myoddweb.directorywatcher.win.x86.dll");
    }

    private string GetFromEmbeddedx64()
    {
      Contract.Assert(Environment.Is64BitProcess);
      return CreateResourceFile("x64.directorywatcher.win", "myoddweb.directorywatcher.win.x64.dll");
    }

    private int Callback(
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

    public override long Start(IRequest request)
    {
      return _helper.Start(request);
    }

    public override bool Stop(long id)
    {
      return _helper.Stop(id);
    }
    #endregion
  }
}
