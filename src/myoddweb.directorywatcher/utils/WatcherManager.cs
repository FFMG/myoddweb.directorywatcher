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
using System.IO;
using System.Reflection;
using System.Security.Cryptography;
using myoddweb.directorywatcher.interfaces;

namespace myoddweb.directorywatcher.utils
{
  internal class WatcherManager
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

    /// <summary>
    /// 'Borrowed' from https://www.codeproject.com/Articles/528178/Load-DLL-From-Embedded-Resource
    /// </summary>
    /// <returns></returns>
    public static string GetInteropResourceFileSystem()
    {
      var winresource = "win32.directorywatcher.win";
      var resource = "win32.directorywatcher.interop";
      if (Environment.Is64BitProcess)
      {
        resource = "x64.directorywatcher.interop";
        winresource = "x64.directorywatcher.win";
      }

      var asmwin = GetInteropResourceFileSystem(winresource, "myoddweb.directorywatcher.win.dll");

      const string actualDllFilename = "myoddweb.directorywatcher.interop.dll";
      return GetInteropResourceFileSystem(resource, actualDllFilename);
    }

    /// <summary>
    /// 'Borrowed' from https://www.codeproject.com/Articles/528178/Load-DLL-From-Embedded-Resource
    /// </summary>
    /// <returns></returns>
    public static string GetInteropResourceFileSystem( string resource, string dll )
    {
      byte[] ba;
      var fileOk = false;
      string tempFile;
      var tempPath = Path.Combine(new[] { Path.GetTempPath(), "directorywatcher", Environment.Is64BitProcess ? "x64" : "x86" });

      var curAsm = Assembly.GetExecutingAssembly();
      using (var stm = curAsm.GetManifestResourceStream(resource))
      {
        // Either the file is not existed or it is not mark as embedded resource
        if (stm == null)
          throw new Exception(resource + " is not found in Embedded Resources.");

        // Get byte[] from the file from embedded resource
        ba = new byte[(int)stm.Length];
        stm.Read(ba, 0, (int)stm.Length);
      }

      using (var sha1 = new SHA1CryptoServiceProvider())
      {
        var fileHash = BitConverter.ToString(sha1.ComputeHash(ba)).Replace("-", string.Empty);

        tempFile = Path.Combine(new[] { tempPath, dll });
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
        if (!Directory.Exists(tempPath))
        {
          Directory.CreateDirectory(tempPath);
        }
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
      var directoryName = Directory.GetCurrentDirectory();
      var dllInteropPath = Path.Combine(directoryName, "Win32\\myoddweb.directorywatcher.interop.dll");
      if (Environment.Is64BitProcess)
      {
        dllInteropPath = Path.Combine(directoryName, "x64\\myoddweb.directorywatcher.interop.dll");
      }

      return dllInteropPath;
    }

    /// <summary>
    /// Create the watcher from the file system, throw if we are unable to do it.
    /// </summary>
    /// <returns></returns>
    private static IWatcher1 CreateWatcherFromFileSystem()
    {
      try
      {
        try
        {
          var asm = TypeLoader.LoadFromFile(GetInteropResourceFileSystem());
          // var asm = TypeLoader.LoadFromFile(GetInteropFromFileSystem());
          return TypeLoader.LoadTypeFromAssembly<IWatcher1>(asm);
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
