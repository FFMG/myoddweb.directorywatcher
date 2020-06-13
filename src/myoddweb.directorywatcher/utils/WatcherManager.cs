// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
using myoddweb.directorywatcher.interfaces;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;
using myoddweb.directorywatcher.utils.Helper;

namespace myoddweb.directorywatcher.utils
{
  internal abstract class WatcherManager : IDisposable
  {
    #region Member variables
    /// <summary>
    /// The dictionary with all the events.
    /// </summary>
    private readonly Dictionary<long, IList<IEvent>> _idAndEvents = new Dictionary<long, IList<IEvent>>();

    /// <summary>
    /// Dictionary with the statistics
    /// </summary>
    private readonly Dictionary<long, IStatistics> _idStats = new Dictionary<long, IStatistics>();

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
    
    #region Events
    public delegate void Watcher<in T>(T e);

    /// <summary>
    /// Non async methon when a message arrives from the logger.
    /// </summary>
    public event Watcher<ILoggerEvent> OnLogger;
    #endregion

    /// <summary>
    /// Private constructor
    /// </summary>
    protected internal WatcherManager()
    {
    }

    #region Protected Methods
    /// <summary>
    /// The callback function for when we receive a message from the directory watcher.
    /// </summary>
    /// <param name="id"></param>
    /// <param name="level"></param>
    /// <param name="message"></param>
    protected void LoggerCallback(
      long id,
      int level,
      string message
    )
    {
      // cast the level from an int to an enum
      // any value will work, even unknown ones.
      OnLogger?.Invoke(new LoggerEvent(id, (LogLevel)level, message));
    }

    protected void StatisticsCallback(
      long id,
      double elapsedTime,
      long numberOfEvents
    )
    {
      lock (_idStats)
      {
        if (!_idStats.ContainsKey(id))
        {
          _idStats[id] = new Statistics(id, elapsedTime, numberOfEvents);
        }
        else
        {
          var statistics = _idStats[id];
          _idStats[id] = new Statistics(
            id,
            statistics.ElapsedTime + statistics.ElapsedTime,
            statistics.NumberOfEvents + numberOfEvents
          );
        }
      }
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
    protected void EventsCallback(
      long id,
      bool isFile,
      string name,
      string oldName,
      int action,
      int error,
      long dateTimeUtc)
    {
      lock (_idAndEvents)
      {
        var e = new Event(
          isFile,
          name,
          oldName,
          (EventAction)action,
          (interfaces.EventError)error,
          DateTime.FromFileTimeUtc(dateTimeUtc)
        );
        if (!_idAndEvents.ContainsKey(id))
        {
          _idAndEvents[id] = new List<IEvent> { e };
        }
        else
        {
          _idAndEvents[id].Add(e);
        }
      }
    }
    #endregion

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
    public bool GetStatistics(long id, out IStatistics statistics)
    {
      lock (_idStats)
      {
        // do we have that data at all?
        if (!_idStats.ContainsKey(id))
        {
          statistics = null;
          return false;
        }

        statistics = _idStats[id];
        _idStats.Remove(id);

        // the value coul be null
        return statistics != null;
      }
    }

    public long GetEvents(long id, out IList<IEvent> events)
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
          e.DateTimeUtc)).ToArray();
        _idAndEvents[id].Clear();
        return events.Count;
      }
    }

    public abstract long Start(IRequest request);

    public abstract bool Stop(long id);
    
    public abstract bool Ready();
    #endregion
  }
}
