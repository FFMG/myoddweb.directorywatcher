// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
using myoddweb.directorywatcher.interfaces;
using System;
using System.Collections.Generic;
using System.IO;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;

namespace myoddweb.directorywatcher.utils
{
  internal abstract class WatcherManager : IDisposable
  {
    #region Member variables
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
    protected string EmbededFolder
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
    #endregion

    /// <summary>
    /// Private constructor
    /// </summary>
    protected internal WatcherManager()
    {
    }

    #region Static helpers
    /// <summary>
    /// Load an embeded file and return the raw data.
    /// 'Borrowed' from https://www.codeproject.com/Articles/528178/Load-DLL-From-Embedded-Resource
    /// </summary>
    /// <returns></returns>
    private static byte[] GetEmbededResource(string resource)
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
    /// Calculate the SHA1 of the given input
    /// </summary>
    /// <param name="input"></param>
    /// <returns></returns>
    static private string Hash(string input)
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
    /// Create a file in the given path with the given data.
    /// </summary>
    /// <param name="path"></param>
    /// <param name="data"></param>
    private static void CreateFile(string path, byte[] data)
    {
      File.WriteAllBytes(path, data);
    }
    #endregion

    #region Private/Protected Methods
    /// <summary>
    /// Clean up resources
    /// </summary>
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
    protected string CreateResourceFile( string resource, string dll )
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
    #endregion

    #region Abstract Methods
    public abstract long Start(IRequest request);

    public abstract bool Stop(long id);

    public abstract long GetEvents(long id, out IList<IEvent> events);

    public abstract bool GetStatistics(long id, out IStatistics statistics);
    
    public abstract bool Ready();
    #endregion
  }
}
