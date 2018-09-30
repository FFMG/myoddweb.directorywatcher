using System;
using System.IO;
using System.Reflection;
using myoddweb.directorywatcher.utils;

namespace myoddweb.directorywatcher
{
  public class Watcher
  {
    public Watcher()
    {
      var directoryName = Directory.GetCurrentDirectory();
      var dllInteropPath = Path.Combine(directoryName, "Win32\\myoddweb.directorywatcher.interop.dll");
      if (Environment.Is64BitProcess)
      {
        dllInteropPath = Path.Combine(directoryName, "x64\\myoddweb.directorywatcher.interop.dll");
      }

      // look for the 
      Assembly asm = null;
      try
      {
        asm = Assembly.LoadFrom(dllInteropPath);
      }
      catch (ArgumentException ex)
      {
        throw new Exception($"The interop file name/path does not appear to be valid. '{dllInteropPath}'.{Environment.NewLine}{Environment.NewLine}{ex.Message}");
      }
      catch (FileNotFoundException ex)
      {
        throw new Exception($"Unable to load the interop file. '{dllInteropPath}'.{Environment.NewLine}{Environment.NewLine}{ex.Message}");
      }
      var watcher = TypeLoader.LoadTypeFromAssembly<Interop.Watcher>(asm);

    }
  }
}
