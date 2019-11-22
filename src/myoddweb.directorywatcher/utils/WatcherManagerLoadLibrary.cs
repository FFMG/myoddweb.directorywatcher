using System;
using System.Collections.Generic;
using System.Diagnostics.Contracts;
using System.IO;
using System.Runtime.InteropServices;
using myoddweb.directorywatcher.interfaces;

namespace myoddweb.directorywatcher.utils
{
  internal static class Delegates
  {
    public struct Request
    {
      [MarshalAs(UnmanagedType.LPWStr)] 
      public string Path;

      [MarshalAs(UnmanagedType.I1)]
      public bool Recursive;
    }

    // Delegate with function signature for the GetVersion function
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    [return: MarshalAs(UnmanagedType.I8)]
    public delegate Int64 Start(ref Request request );

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public delegate bool Stop([In, MarshalAs(UnmanagedType.U8)] Int64 id );
  }

  internal static class NativeLibrary
  {
    [DllImport("kernel32.dll")]
    public static extern IntPtr LoadLibrary(string dllToLoad);

    [DllImport("kernel32.dll")]
    public static extern IntPtr GetProcAddress(IntPtr hModule, string procedureName);

    [DllImport("kernel32.dll")]
    public static extern bool FreeLibrary(IntPtr hModule);
  }

  internal class WatcherManagerLoadLibrary : WatcherManager
  {
    private Delegates.Start _start;
    private Delegates.Stop _stop;

    /// <summary>
    /// The handle of the windows c++ dll ... if loaded.
    /// </summary>
    private readonly IntPtr _handle;

    public WatcherManagerLoadLibrary()
    {
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
    #endregion

    #region Abstract methods
    public override long GetEvents(long id, out IList<IEvent> events)
    {
      throw new System.NotImplementedException();
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
      return _start( ref r );
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
