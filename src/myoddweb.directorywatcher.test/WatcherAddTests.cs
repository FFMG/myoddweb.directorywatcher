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

    [TestCase(0, true)] // if nothing is added, we don't see anything
    [TestCase(1, true)]
    [TestCase(5, true)]
    [TestCase(42, true)] //  we have to...
    [TestCase(0, false)]
    [TestCase(1, false)]
    [TestCase(5, false)]
    [TestCase(42, false)]
    public async Task TestSimpleAddFilesNotification(int numberToAdd, bool recursive)
    {
      // the number of files we will be adding
      var numberAdded = 0;

      // first we need to create the watcher
      using ( var watcher = new Watcher())
      {
        watcher.Add(new Request(_tempDirectory, recursive));
        watcher.OnAddedAsync += ( fse,  token) =>
          {
            TestContext.Out.WriteLine("Message to write to log");
            if (fse.IsFile)
            {
              ++numberAdded;
            }
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
