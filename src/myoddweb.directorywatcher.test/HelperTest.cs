using System;
using System.IO;

namespace myoddweb.directorywatcher.test
{
  internal class HelperTest : IDisposable
  {
    public string Folder { get; }

    public HelperTest()
    {
      Folder = GetTemporaryDirectory();
    }

    private string GetTemporaryDirectory()
    {
      string tempDirectory = Path.Combine(Path.GetTempPath(), $"test.{Path.GetRandomFileName()}");
      Directory.CreateDirectory(tempDirectory);
      return tempDirectory;
    }

    public void Dispose()
    {
      try
      {
        Directory.Delete(Folder, true );
      }
      catch
      {
        // ignore
      }
    }
    
    public void AddFile()
    {
      string tempFile = Path.Combine(Folder, Path.GetRandomFileName());
      using (File.Create(tempFile)) { };
    }

    public string AddFolder()
    {
      string tempDirectory = Path.Combine(Folder, $"test.sub.{Path.GetRandomFileName()}");
      Directory.CreateDirectory(tempDirectory);
      return tempDirectory;
    }
  }
}
