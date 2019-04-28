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
using System;
using System.Diagnostics.Contracts;
using System.IO;
using System.Reflection;
using System.Security.Cryptography;
using myoddweb.directorywatcher.interfaces;

namespace myoddweb.directorywatcher.utils
{
  internal class WatcherManager : IDisposable
  {
    /// <summary>
    /// The actual instance of the watcher.
    /// </summary>
    private readonly IWatcher1 _watcher;

    /// <summary>
    /// The one and only instance of the manager.
    /// </summary>
    private static WatcherManager _manager;

    /// <summary>
    /// The object we will use for the lock.
    /// </summary>
    private static readonly object Lock = new object();

    /// <summary>
    /// The folder where our embeded files are located.
    /// </summary>
    private string _embededFolder;

    /// <summary>
    /// Get the current embeded folder.
    /// </summary>
    private string EmbededFolder
    {
      get
      {
        if (_embededFolder != null)
        {
          return _embededFolder;
        }

        lock (Lock)
        {
          if (null != _embededFolder)
          {
            return _embededFolder;
          }

          // remove the old directories.
          RemoveOldDirectories();

          // create the new folder.
          var guid = Guid.NewGuid().ToString();
          var embededFolder = Path.Combine(new[] { Path.GetTempPath(), $"wr.{guid}" });
          if (!Directory.Exists(embededFolder))
          {
            Directory.CreateDirectory(embededFolder);
          }

          // last chance, either return what we have or simply set the value.
          return _embededFolder ?? (_embededFolder = embededFolder);
        }
      }
    }

    /// <summary>
    /// Check if we have disposed of the instance or not.
    /// </summary>
    private bool _disposed;

    /// <summary>
    /// Get our one and only watcher interface.
    /// </summary>
    public static IWatcher1 Get => Instance._watcher;

    /// <summary>
    /// The one and only instance of this class.
    /// </summary>
    private static WatcherManager Instance
    {
      get
      {
        // do we already have the instance?
        if (null != _manager)
        {
          return _manager;
        }

        // check again using the lock
        // otherwise just create it.
        lock (Lock)
        {
          // either return what we have or simply create a new one.
          return _manager ?? (_manager = new WatcherManager());
        }
      }
    }

    /// <summary>
    /// Private constructor
    /// </summary>
    private WatcherManager()
    {
      // create the one watcher.
      _watcher = CreateWatcherFromFileSystem();
    }

    public void Dispose()
    {
      // done already?
      if (_disposed )
      {
        return;
      }

      _disposed = true;
      if (_embededFolder == null)
      {
        return;
      }

      lock (Lock)
      {
        if (_embededFolder == null)
        {
          return;
        }

        // reset the folder name
        _embededFolder = null;

        // we are done with the instance as well
        _manager = null;

        // remove the old directories if we can.
        RemoveOldDirectories();
      }
    }

    /// <summary>
    /// Clean up old directories.
    /// </summary>
    private static void RemoveOldDirectories()
    {
      var directories = Directory.GetDirectories(Path.GetTempPath(), "wr.*");
      foreach (var directory in directories)
      {
        try
        {
          Directory.Delete(directory, true);
        }
        catch
        {
          // we cannot do much about this here.
        }
      }
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
      const string winresource = "win32.directorywatcher.win";
      const string resource = "win32.directorywatcher.interop";
      var asmwin = GetInteropResourceFileSystem(winresource, "myoddweb.directorywatcher.win.x86.dll");
      const string actualDllFilename = "myoddweb.directorywatcher.interop.x86.dll";
      return GetInteropResourceFileSystem(resource, actualDllFilename);
    }

    private string GetInteropResourceFileSystemx64()
    {
      const string resource = "x64.directorywatcher.interop";
      const string winresource = "x64.directorywatcher.win";
      var asmwin = GetInteropResourceFileSystem(winresource, "myoddweb.directorywatcher.win.x64.dll");
      const string actualDllFilename = "myoddweb.directorywatcher.interop.x64.dll";
      return GetInteropResourceFileSystem(resource, actualDllFilename);
    }

    /// <summary>
    /// 'Borrowed' from https://www.codeproject.com/Articles/528178/Load-DLL-From-Embedded-Resource
    /// </summary>
    /// <returns></returns>
    private byte[] GetEmbededResource(string resource )
    {
      var curAsm = Assembly.GetExecutingAssembly();
      using (var stm = curAsm.GetManifestResourceStream(resource))
      {
        // Either the file is not existed or it is not mark as embedded resource
        if (stm == null)
        {
          throw new Exception(resource + " is not found in Embedded Resources.");
        }

        // Get byte[] from the file from embedded resource
        var ba = new byte[(int)stm.Length];
        stm.Read(ba, 0, (int)stm.Length);
        return ba;
      }
    }

    /// <summary>
    /// 'Borrowed' from https://www.codeproject.com/Articles/528178/Load-DLL-From-Embedded-Resource
    /// </summary>
    /// <returns></returns>
    private string GetInteropResourceFileSystem( string resource, string dll )
    {
      //  we throw in the function if we cannot locate the data.
      var ba = GetEmbededResource( resource);
      var fileOk = false;
      string tempFile;

      using (var sha1 = new SHA1CryptoServiceProvider())
      {
        var fileHash = BitConverter.ToString(sha1.ComputeHash(ba)).Replace("-", string.Empty);

        tempFile = Path.Combine(new[] { EmbededFolder, dll });
        if (File.Exists(tempFile))
        {
          var bb = File.ReadAllBytes(tempFile);
          var fileHash2 = BitConverter.ToString(sha1.ComputeHash(bb)).Replace("-", string.Empty);

          if (fileHash == fileHash2)
          {
            fileOk = true;
          }
        }
      }

      if (!fileOk)
      {
        File.WriteAllBytes(tempFile, ba);
      }
      return tempFile;
    }

    /// <summary>
    /// Get the interop path on file.
    /// </summary>
    /// <returns></returns>
    private static string GetInteropFromFileSystem()
    {
      return Environment.Is64BitProcess ? GetInteropFromFileSystemx64() : GetInteropFromFileSystemx86();
    }

    private static string GetInteropFromFileSystemx86()
    {
      Contract.Assert(!Environment.Is64BitProcess);
      var directoryName = Directory.GetCurrentDirectory();
      return Path.Combine(directoryName, "Win32\\myoddweb.directorywatcher.interop.x86.dll");
    }

    private static string GetInteropFromFileSystemx64()
    {
      Contract.Assert(Environment.Is64BitProcess);
      var directoryName = Directory.GetCurrentDirectory();
      return Path.Combine(directoryName, "x64\\myoddweb.directorywatcher.interop.x64.dll");
    }

    /// <summary>
    /// Create the watcher from the file system, throw if we are unable to do it.
    /// </summary>
    /// <returns></returns>
    private IWatcher1 CreateWatcherFromFileSystem()
    {
      try
      {
        try
        {
#if DEBUG
          // while we debug the assembly we want to have access to the files
          // so it is better to load from file system.
          // otherwise we will load from the file system.
          var assemblyFilePath = GetInteropFromFileSystem();
          return TypeLoader.LoadTypeFromAssembly<IWatcher1>(assemblyFilePath);
#else
          try
          {
            // try and get the files from the file system.
            var assemblyFilePath = GetInteropResourceFileSystem();
            return TypeLoader.LoadTypeFromAssembly<IWatcher1>(assemblyFilePath);
          }
          catch
          {
            // something broke ... try and load from the embeded files.
            // we will throw again if there is a further problem...
            var assemblyFilePath = GetInteropFromFileSystem();
            return TypeLoader.LoadTypeFromAssembly<IWatcher1>(assemblyFilePath);
          }
#endif
        }
        catch (Exception e)
        {
          Console.WriteLine(e);
          throw;
        }
      }
      catch (ArgumentException ex)
      {
        throw new Exception($"The interop file name/path does not appear to be valid. '{GetInteropFromFileSystem()}'.{Environment.NewLine}{Environment.NewLine}{ex.Message}");
      }
      catch (FileNotFoundException ex)
      {
        throw new Exception($"Unable to load the interop file. '{ex.FileName}'.{Environment.NewLine}{Environment.NewLine}{ex.Message}");
      }
    }
  }
}
