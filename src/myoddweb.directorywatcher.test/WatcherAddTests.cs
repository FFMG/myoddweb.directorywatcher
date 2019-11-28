using NUnit.Framework;
using System.IO;
using System.Threading;
using System.Threading.Tasks;

namespace myoddweb.directorywatcher.test
{
  [TestFixture]
  internal class WatcherAddTests
  {
    private string _tempDirectory;
    [SetUp]
    public void Setup()
    {
      // create the folder that we will be working in
      _tempDirectory = Path.Combine(Path.GetTempPath(), Path.GetRandomFileName());
      Directory.CreateDirectory(_tempDirectory);
    }

    [TearDown]
    public void TearDown()
    {
      var di = new DirectoryInfo(_tempDirectory);
      foreach (var file in di.GetFiles())
      {
        try
        {
          file.Delete();
        }
        catch
        {
          //nothing
        }
      }
      foreach (var dir in di.GetDirectories())
      {
        dir.Delete(true);
      }
    }

    [TestCase(0)] // if nothing is added, we don't see anything
    [TestCase(1)]
    [TestCase(5)]
    [TestCase(42)] //  we have to...
    public async Task TestSimpleAddFileNotification( int numberToAdd)
    {
      // the number of files we will be adding
      var numberAdded = 0;

      // first we need to create the watcher
      using ( var watcher = new Watcher())
      {
        watcher.Add(new Request(_tempDirectory, true));
        watcher.OnAddedAsync += ( fse,  token) =>
          {
            ++numberAdded;
            return Task.CompletedTask;
          };
        watcher.Start();
        for (var i = 0; i < numberToAdd; ++i)
        {
          // create an empty file
          var filename = Path.Combine(_tempDirectory, Path.GetRandomFileName());
          await using (File.Create(filename)) { }
          await Task.Yield();
        }

        // give a bit of time
        SpinWait.SpinUntil( () => numberAdded >= numberToAdd, 10000);
      }
      Assert.AreEqual(numberToAdd, numberAdded);
    }
  }
}
