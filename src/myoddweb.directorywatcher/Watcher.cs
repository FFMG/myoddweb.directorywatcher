using System;
using System.IO;
using System.Reflection;
using myoddweb.directorywatcher.utils;
using myoddweb.directorywatcher.interfaces;

namespace myoddweb.directorywatcher
{
  public class Watcher : IWatcher1, IDisposable
  {
    /// <summary>
    /// The actual watcher.
    /// </summary>
    private readonly IWatcher1 _watcher;

    public Watcher()
    {
      _watcher = CreateWatcher();
    }

    public void Dispose()
    {
    }

    /// <summary>
    /// Create the watcher, throw if we are unable to do it.
    /// </summary>
    /// <returns></returns>
    private static IWatcher1 CreateWatcher()
    {
      var directoryName = Directory.GetCurrentDirectory();
      var dllInteropPath = Path.Combine(directoryName, "Win32\\myoddweb.directorywatcher.interop.dll");
      if (Environment.Is64BitProcess)
      {
        dllInteropPath = Path.Combine(directoryName, "x64\\myoddweb.directorywatcher.interop.dll");
      }

      // look for the 
      try
      {
        var asm = Assembly.LoadFrom(dllInteropPath);
        return TypeLoader.LoadTypeFromAssembly<IWatcher1>(asm);
      }
      catch (ArgumentException ex)
      {
        throw new Exception($"The interop file name/path does not appear to be valid. '{dllInteropPath}'.{Environment.NewLine}{Environment.NewLine}{ex.Message}");
      }
      catch (FileNotFoundException ex)
      {
        throw new Exception($"Unable to load the interop file. '{dllInteropPath}'.{Environment.NewLine}{Environment.NewLine}{ex.Message}");
      }
    }

    /// <inheritdoc />
    public long Monitor(string path, bool recursive)
    {
      return _watcher?.Monitor(path, recursive) ?? -1;
    }
  }
}
