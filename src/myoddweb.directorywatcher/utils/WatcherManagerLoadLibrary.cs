using System;
using System.Collections.Generic;
using System.Diagnostics.Contracts;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using myoddweb.directorywatcher.interfaces;
using myoddweb.directorywatcher.utils.Helper;

namespace myoddweb.directorywatcher.utils
{
  internal class WatcherManagerLoadLibrary : WatcherManager
  {
    /// <summary>
    /// The delegate to start a request.
    /// </summary>
    private Delegates.Start _start;

    /// <summary>
    /// Delegate to stop a certain request
    /// </summary>
    private Delegates.Stop _stop;

    /// <summary>
    /// The callback function called from time to time when Events happen.
    /// </summary>
    private readonly Delegates.Callback _callback = new Delegates.Callback(Callback);

    /// <summary>
    /// The dictionary with all the events.
    /// </summary>
    private static Dictionary<long, IList<IEvent>> _idAndEvents = new Dictionary<long, IList<IEvent>>();

    /// <summary>
    /// The handle of the windows c++ dll ... if loaded.
    /// </summary>
    private readonly IntPtr _handle;

    public WatcherManagerLoadLibrary()
    {
      // Create the file handle. 
      // we will throw if this does not exist.
      _handle = CreateWatcherFromFileSystem();
    }

    ~WatcherManagerLoadLibrary()
    {
      if (_handle != IntPtr.Zero)
      {
        NativeLibrary.FreeLibrary(_handle);
      }
    }

    #region Private Methods
    private IntPtr CreateWatcherFromFileSystem()
    {
      var library = GetFromFileSystem();
      return NativeLibrary.LoadLibrary(library);
    }

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
      return GetInteropFromFileSystem("Win32", "myoddweb.directorywatcher.win.x86.dll");
    }

    private static string GetFromFileSystemx64()
    {
      Contract.Assert(Environment.Is64BitProcess);
      return GetInteropFromFileSystem("x64", "myoddweb.directorywatcher.win.x64.dll");
    }

    private static string GetInteropFromFileSystem(string subDirectory, string dll)
    {
      var currentDirectory = Path.Combine(Directory.GetCurrentDirectory(), subDirectory);
      if (!Directory.Exists(currentDirectory))
      {
        var parentCurrentDirectory = (new DirectoryInfo(Directory.GetCurrentDirectory())).Parent.FullName;
        currentDirectory = Path.Combine(parentCurrentDirectory, subDirectory);
      }

      return Path.Combine(currentDirectory, dll);
    }

    private T Get<T>(string name ) where T: class
    {
      if (_handle == IntPtr.Zero)
      {
        throw new NotImplementedException();
      }
      var start_handle = NativeLibrary.GetProcAddress(_handle, name);
      if (start_handle == IntPtr.Zero)
      {
        throw new NotImplementedException();
      }
      return Marshal.GetDelegateForFunctionPointer(
          start_handle,
          typeof(T)) as T;

    }

    private static int Callback(
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
      if (_start == null)
      {
        _start = Get<Delegates.Start>("Start");
      }
      Delegates.Request r = new Delegates.Request
      {
        Recursive = request.Recursive,
        Path = request.Path
      };

      // start
      return _start( ref r, _callback, 1000 );
    }

    public override bool Stop(long id)
    {
      if (_stop == null)
      {
        _stop = Get<Delegates.Stop>("Stop");
      }
      return _stop( id );
    }
    #endregion
  }

}
