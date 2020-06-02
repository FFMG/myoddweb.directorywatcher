using myoddweb.directorywatcher.interfaces;
using System;
using System.Runtime.InteropServices;

namespace myoddweb.directorywatcher.utils.Helper
{
  internal class WatcherManagerNativeLibrary
  {
    /// <summary>
    /// If the watch manager is ready or not.
    /// </summary>
    private Delegates.Ready _ready;

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
    private readonly Delegates.Callback _callback;

    /// <summary>
    /// The handle of the windows c++ dll ... if loaded.
    /// </summary>
    private readonly IntPtr _handle;

    public WatcherManagerNativeLibrary(string library, Delegates.Callback callback)
    {
      // some sanity checks.
      if( library == null )
      {
        throw new ArgumentNullException(nameof(library));
      }

      _handle = CreatePtrFromFileSystem( library );
      _callback = callback ?? throw new ArgumentNullException(nameof(callback));
    }

    ~WatcherManagerNativeLibrary()
    {
      if (_handle != IntPtr.Zero)
      {
#if MYODDWEB_NETCOREAPP
        System.Runtime.InteropServices.NativeLibrary.Free(_handle);
#else
        _ = NativeLibrary.FreeLibrary(_handle);
#endif
      }
    }

    private static IntPtr CreatePtrFromFileSystem( string library )
    {
#if MYODDWEB_NETCOREAPP
      return System.Runtime.InteropServices.NativeLibrary.Load(library);
#else
      return NativeLibrary.LoadLibrary(library);
#endif
    }

    private T Get<T>(string name) where T : class
    {
      if (_handle == IntPtr.Zero)
      {
        throw new NotImplementedException();
      }
#if MYODDWEB_NETCOREAPP
      var start_handle = System.Runtime.InteropServices.NativeLibrary.GetExport(_handle, name);
#else
      var start_handle = NativeLibrary.GetProcAddress(_handle, name);
#endif
      if (start_handle == IntPtr.Zero)
      {
        throw new NotImplementedException();
      }
      return Marshal.GetDelegateForFunctionPointer(
          start_handle,
          typeof(T)) as T;

    }

    public long Start(IRequest request)
    {
      if (_start == null)
      {
        _start = Get<Delegates.Start>("Start");
      }
      var requestDelegatedelegate = new Delegates.Request
      {
        Recursive = request.Recursive,
        Path = request.Path,
        Callback = _callback,
        CallbackIntervalMs = 50
      };

      // start
      return _start(ref requestDelegatedelegate);
    }

    public bool Stop(long id)
    {
      if (_stop == null)
      {
        _stop = Get<Delegates.Stop>("Stop");
      }
      return _stop(id);
    }

    public bool Ready()
    {
      if (_ready == null)
      {
        _ready = Get<Delegates.Ready>("Ready");
      }
      return _ready();
    }
  }
}
