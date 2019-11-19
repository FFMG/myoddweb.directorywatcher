// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
using System;
using System.Collections.Generic;
using System.Diagnostics.Contracts;
using System.IO;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;
using myoddweb.directorywatcher.interfaces;

namespace myoddweb.directorywatcher.utils
{
  internal class WatcherManager : IDisposable
  {
    /// <summary>
    /// The actual instance of the watcher.
    /// </summary>
    public IWatcher1 Watcher { get; }

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

          // use the assembly name to create a somewhat unique path
          var curAsm = Assembly.GetExecutingAssembly();

          // create the new folder.
          var sha1 = Hash( curAsm.FullName );
          var embededFolder = Path.Combine(new[] { Path.GetTempPath(), $"wr.{sha1}" });
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
    /// If we want to load the embeded resource or if we wish to try and load
    /// The file from a reference.
    /// </summary>
    private readonly bool _loadEmbeddedResource;
    
    /// <summary>
    /// Private constructor
    /// </summary>
    internal WatcherManager(bool loadEmbeddedResource)
    {
      // load from ref or fro embeded resource
      _loadEmbeddedResource = loadEmbeddedResource;

      // create the one watcher.
      Watcher = CreateWatcherFromFileSystem();
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

        // dispose our own directory if we can
        DisposeOwnDirectory();
      }
    }

    /// <summary>
    /// Calculate the SHA1 of the given input
    /// </summary>
    /// <param name="input"></param>
    /// <returns></returns>
    static string Hash(string input)
    {
      using (var sha1 = new SHA1CryptoServiceProvider())
      {
        var hash = sha1.ComputeHash(Encoding.UTF8.GetBytes(input));
        var sb = new StringBuilder(hash.Length * 2);
        foreach (byte b in hash)
        {
          sb.Append(b.ToString("x2"));
        }
        return sb.ToString();
      }
    }

    /// <summary>
    /// Clean up old directories.
    /// </summary>
    private void DisposeOwnDirectory()
    {
      if (_embededFolder == null)
      {
        return;
      }

      try
      {
        Directory.Delete(_embededFolder, true);
      }
      catch
      {
        // assemblies are not unloaded by us
        // so it is posible that the file is locked.
      }
      finally
      {
        // reset the folder name
        _embededFolder = null;
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
      // first we try and copy the win
      const string winResource = "win32.directorywatcher.win";
      const string winDllFilename = "myoddweb.directorywatcher.win.x86.dll";
      CreateResourceFile(winResource, winDllFilename);

      // then copy the interop
      const string interopResource = "win32.directorywatcher.interop";
      const string interopDllFilename = "myoddweb.directorywatcher.interop.x86.dll";
      return CreateResourceFile(interopResource, interopDllFilename);
    }

    private string GetInteropResourceFileSystemx64()
    {
      // first we try and copy the win
      const string winResource = "x64.directorywatcher.win";
      const string winDllFilename = "myoddweb.directorywatcher.win.x64.dll";
      CreateResourceFile(winResource, winDllFilename);

      const string interopResource = "x64.directorywatcher.interop";
      const string interopDllFilename = "myoddweb.directorywatcher.interop.x64.dll";
      return CreateResourceFile(interopResource, interopDllFilename);
    }

    /// <summary>
    /// 'Borrowed' from https://www.codeproject.com/Articles/528178/Load-DLL-From-Embedded-Resource
    /// </summary>
    /// <returns></returns>
    private static byte[] GetEmbededResource(string resource )
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
        var ba = new byte[stm.Length];
        stm.Read(ba, 0, ba.Length);
        return ba;
      }
    }

    /// <summary>
    /// 'Borrowed' from https://www.codeproject.com/Articles/528178/Load-DLL-From-Embedded-Resource
    /// </summary>
    /// <returns></returns>
    private string CreateResourceFile( string resource, string dll )
    {
      //  we throw in the function if we cannot locate the data.
      var ba = GetEmbededResource( resource);

      // get the temp file we will be using.
      var tempFile = Path.Combine(new[] { EmbededFolder, dll });
      if (!File.Exists(tempFile))
      {
        // the file does not exist, create it.
        // and return the path
        CreateFile(tempFile, ba);
        return tempFile;
      }

      // it already exists, do compare the current value
      // and then try and replace the old with new value.
      using (var sha1 = new SHA1CryptoServiceProvider())
      {
        // get the current hash
        var fileHash = BitConverter.ToString(sha1.ComputeHash(ba)).Replace("-", string.Empty);

        // then get the 'new'
        var bb = File.ReadAllBytes(tempFile);
        var fileHash2 = BitConverter.ToString(sha1.ComputeHash(bb)).Replace("-", string.Empty);

        // compare the two hashes
        if (fileHash == fileHash2)
        {
          // both hashes are the same, no need to replace anything
          return tempFile;
        }
      }

      // we need to replace the current file
      // if this throws it means that another process ... with the same name as us
      // is using a different resource as us...
      CreateFile(tempFile, ba);
      return tempFile;
    }

    private static void CreateFile( string path, byte[] data )
    {
      File.WriteAllBytes(path, data);
    }

    /// <summary>
    /// Get the interop path on file.
    /// </summary>
    /// <returns></returns>
    private static string GetInteropFromFileSystem()
    {
      return Environment.Is64BitProcess ? GetInteropFromFileSystemx64() : GetInteropFromFileSystemx86();
    }

    private static string GetInteropFromFileSystem( string subDirectory, string dll )
    {
      var currentDirectory = Path.Combine(Directory.GetCurrentDirectory(), subDirectory );
      if (!Directory.Exists(currentDirectory))
      {
        var parentCurrentDirectory = (new DirectoryInfo(Directory.GetCurrentDirectory())).Parent.FullName;
        currentDirectory = Path.Combine(parentCurrentDirectory, subDirectory );
      }

      return Path.Combine(currentDirectory, dll );
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
        try
        {
          if (!_loadEmbeddedResource)
          {
            // while we debug the assembly we want to have access to the files
            // so it is better to load from file system.
            // otherwise we will load from the file system.
            var assemblyFilePath = GetInteropFromFileSystem();
            return TypeLoader.LoadTypeFromAssembly<IWatcher1>(assemblyFilePath);
          }
          
          try
          {
            // try and get the files from the file system.
            var assemblyFilePath = GetInteropResourceFileSystem();
            return TypeLoader.LoadTypeFromAssembly<IWatcher1>(assemblyFilePath);
          }
          catch( Exception e )
          {
            // save the inner exception
            innerExceptions.Add(e);

            // something broke ... try and load from the embeded files.
            // we will throw again if there is a further problem...
            var assemblyFilePath = GetInteropFromFileSystem();
            return TypeLoader.LoadTypeFromAssembly<IWatcher1>(assemblyFilePath);
          }
        }
        catch (Exception e)
        {
          innerExceptions.Add( e );
        }
      }
      catch (ArgumentException ex)
      {
        innerExceptions.Add( new Exception($"The interop file name/path does not appear to be valid. '{GetInteropFromFileSystem()}'.{Environment.NewLine}{Environment.NewLine}{ex.Message}"));
      }
      catch (FileNotFoundException ex)
      {
        innerExceptions.Add( new Exception($"Unable to load the interop file. '{ex.FileName}'.{Environment.NewLine}{Environment.NewLine}{ex.Message}"));
      }

      // throw all our exceptions.
      throw new AggregateException( innerExceptions );
    }
  }
}
