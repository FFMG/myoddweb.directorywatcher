using System;
using System.Collections.Generic;
using System.Diagnostics.Contracts;
using System.IO;
using System.Linq;
using myoddweb.directorywatcher.interfaces;
using myoddweb.directorywatcher.utils.Helper;

namespace myoddweb.directorywatcher.utils
{
  internal class WatcherManagerLoadLibrary : WatcherManager
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

    public WatcherManagerLoadLibrary()
    {
      // Create helper we will throw if the file does not exist.
      _helper = new WatcherManagerNativeLibrary(GetFromFileSystem(), Callback );
    }

    #region Private Methods
    /// <summary>
    /// Get the windows c++ path on file.
    /// </summary>
    /// <returns></returns>
    private static string GetFromFileSystem()
    {
      return Environment.Is64BitProcess ? GetFromFileSystemx64() : GetFromFileSystemx86();
    }

    private static string GetFromFileSystemx86()
    {
      Contract.Assert(!Environment.Is64BitProcess);
      return GetFromFileSystem("Win32", "myoddweb.directorywatcher.win.x86.dll");
    }

    private static string GetFromFileSystemx64()
    {
      Contract.Assert(Environment.Is64BitProcess);
      return GetFromFileSystem("x64", "myoddweb.directorywatcher.win.x64.dll");
    }

    /// <summary>
    /// Get a dll from a given directory, we try and look for the file in sub directories first
    /// Then we look in the parent/sub directory
    /// As this is mostly used for debug, the format is either
    ///         /x64/my.dll
    ///         /Win32/my.dll
    ///      or
    ///         /NetCoreApp3.0/
    ///         /x64/my.dll
    ///         /Win32/my.dll
    ///         
    ///      In the first case we are alreadt at the parent level
    ///      In the second care we have to go back one step and then look for the dll.
    /// </summary>
    /// <param name="subDirectory"></param>
    /// <param name="dll"></param>
    /// <returns></returns>
    private static string GetFromFileSystem(string subDirectory, string dll)
    {
      var currentDirectory = Path.Combine(Directory.GetCurrentDirectory(), subDirectory);
      if (!Directory.Exists(currentDirectory))
      {
        var parentCurrentDirectory = (new DirectoryInfo(Directory.GetCurrentDirectory())).Parent.FullName;
        currentDirectory = Path.Combine(parentCurrentDirectory, subDirectory);
      }
      return Path.Combine(currentDirectory, dll);
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
    private int Callback(
      long id,
      bool isFile,
      string name,
      string oldName,
      int action,
      int error,
      long dateTimeUtc)
    {
      lock(_idAndEvents)
      {
        if( !_idAndEvents.ContainsKey(id))
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
         e.DateTimeUtc )).ToArray();
        _idAndEvents[id].Clear();
        return events.Count;
      }
    }

    public override long Start(IRequest request)
    {
      return _helper.Start( request );
    }

    public override bool Stop(long id)
    {
      return _helper.Stop( id );
    }

    public override bool Ready()
    {
      return _helper.Ready();
    }
    #endregion
  }
}
