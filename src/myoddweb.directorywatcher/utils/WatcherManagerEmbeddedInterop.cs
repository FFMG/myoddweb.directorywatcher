using myoddweb.directorywatcher.interfaces;
using System;
using System.Collections.Generic;
using System.IO;

namespace myoddweb.directorywatcher.utils
{
  internal class WatcherManagerEmbeddedInterop : WatcherManager
  {
    /// <summary>
    /// The actual instance of the watcher.
    /// </summary>
    private IWatcher1 Watcher { get; }

    internal WatcherManagerEmbeddedInterop()
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
        // try and get the files from the file system.
        var assemblyFilePath = GetInteropResourceFileSystem();
        return TypeLoader.LoadTypeFromAssembly<IWatcher1>(assemblyFilePath);
      }
      catch (ArgumentException ex)
      {
        innerExceptions.Add(new Exception($"The interop file name/path does not appear to be valid. '{EmbededFolder}'.{Environment.NewLine}{Environment.NewLine}{ex.Message}"));
      }
      catch (FileNotFoundException ex)
      {
        innerExceptions.Add(new Exception($"Unable to load the interop file. '{ex.FileName}'.{Environment.NewLine}{Environment.NewLine}{ex.Message}"));
      }
      catch (Exception e)
      {
        innerExceptions.Add(e);
      }

      // throw all our exceptions.
      throw new AggregateException(innerExceptions);
    }

    /// <summary>
    /// 'Borrowed' from https://www.codeproject.com/Articles/528178/Load-DLL-From-Embedded-Resource
    /// </summary>
    /// <returns></returns>
    private string GetInteropResourceFileSystem()
    {
      return Environment.Is64BitProcess ? GetInteropResourceFileSystemx64() : GetInteropResourceFileSystemx86();
    }

    private string GetInteropResourceFileSystemx86()
    {
      // we need to load the needed windows file
      // use a dictionary in case we are going to add more files
      var winDlls = new Dictionary<string, string>
      {
        //  windows
        { "x86.directorywatcher.win", "myoddweb.directorywatcher.win.x86.dll"},
        { "x86.redist.vcruntime140", "vcruntime140.dll"},
        { "x86.redist.vccorlib140", "vccorlib140.dll"},
        { "x86.redist.msvcp140", "msvcp140.dll"},
        { "x86.redist.msvcp140_1", "msvcp140_1.dll"},
        { "x86.redist.msvcp140_2", "msvcp140_2.dll"},
      };
      foreach (var winDll in winDlls)
      {
        CreateResourceFile(winDll.Key, winDll.Value);
      }

      // then copy the interop
      const string interopResource = "x86.directorywatcher.interop";
      const string interopDllFilename = "myoddweb.directorywatcher.interop.x86.dll";
      return CreateResourceFile(interopResource, interopDllFilename);
    }

    private string GetInteropResourceFileSystemx64()
    {
      // we need to load the needed windows file
      // use a dictionary in case we are going to add more files
      var winDlls = new Dictionary<string, string>
      {
        //  windows
        { "x64.directorywatcher.win", "myoddweb.directorywatcher.win.x64.dll"},
        { "x64.redist.vcruntime140", "vcruntime140.dll"},
        { "x64.redist.vcruntime140_1", "vcruntime140_1.dll"},
        { "x64.redist.vccorlib140", "vccorlib140.dll"},
        { "x64.redist.msvcp140", "msvcp140.dll"},
        { "x64.redist.msvcp140_1", "msvcp140_1.dll"},
        { "x64.redist.msvcp140_2", "msvcp140_2.dll"},
      };
      foreach (var winDll in winDlls)
      {
        CreateResourceFile(winDll.Key, winDll.Value);
      }

      // finally get and return the interio.
      const string interopResource = "x64.directorywatcher.interop";
      const string interopDllFilename = "myoddweb.directorywatcher.interop.x64.dll";
      return CreateResourceFile(interopResource, interopDllFilename);
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
