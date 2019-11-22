using myoddweb.directorywatcher.interfaces;
using System;
using System.Collections.Generic;
using System.Diagnostics.Contracts;
using System.IO;

namespace myoddweb.directorywatcher.utils
{
  internal class WatcherManagerInterop : WatcherManager
  {
    /// <summary>
    /// The actual instance of the watcher.
    /// </summary>
    private IWatcher1 Watcher { get; }

    internal WatcherManagerInterop()
    {
      // create the one watcher.
      Watcher = CreateWatcherFromFileSystem();
    }

    #region Private Functions
    /// <summary>
    /// Create the watcher from the file system, throw if we are unable to do it.
    /// </summary>
    /// <returns></returns>
    private IWatcher1 CreateWatcherFromFileSystem()
    {
      // the various exceptions we might get
      var innerExceptions = new List<Exception>();

      try
      {
        // while we debug the assembly we want to have access to the files
        // so it is better to load from file system.
        // otherwise we will load from the file system.
        var assemblyFilePath = GetInteropFromFileSystem();
        return TypeLoader.LoadTypeFromAssembly<IWatcher1>(assemblyFilePath);
      }
      catch (ArgumentException ex)
      {
        innerExceptions.Add(new Exception($"The interop file name/path does not appear to be valid. '{GetInteropFromFileSystem()}'.{Environment.NewLine}{Environment.NewLine}{ex.Message}"));
      }
      catch (FileNotFoundException ex)
      {
        innerExceptions.Add(new Exception($"Unable to load the interop file. '{ex.FileName}'.{Environment.NewLine}{Environment.NewLine}{ex.Message}"));
      }
      catch (Exception e)
      {
        // save the inner exception
        innerExceptions.Add(e);

        // something broke ... try and load from the embeded files.
        // we will throw again if there is a further problem...
        var assemblyFilePath = GetInteropFromFileSystem();
        return TypeLoader.LoadTypeFromAssembly<IWatcher1>(assemblyFilePath);
      }

      // throw all our exceptions.
      throw new AggregateException(innerExceptions);
    }

    /// <summary>
    /// Get the interop path on file.
    /// </summary>
    /// <returns></returns>
    private static string GetInteropFromFileSystem()
    {
      return Environment.Is64BitProcess ? GetInteropFromFileSystemx64() : GetInteropFromFileSystemx86();
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

    private static string GetInteropFromFileSystemx86()
    {
      Contract.Assert(!Environment.Is64BitProcess);
      return GetInteropFromFileSystem("Win32", "myoddweb.directorywatcher.interop.x86.dll");
    }

    private static string GetInteropFromFileSystemx64()
    {
      Contract.Assert(Environment.Is64BitProcess);
      return GetInteropFromFileSystem("x64", "myoddweb.directorywatcher.interop.x64.dll");
    }
    #endregion

    #region Abstract Methods
    override public long Start(IRequest request)
    {
      return Watcher.Start(request);
    }

    override public bool Stop(long id)
    {
      return Watcher.Stop(id);
    }

    override public long GetEvents(long id, out IList<IEvent> events)
    {
      return Watcher.GetEvents(id, out events);
    }
    #endregion  
  }
}
