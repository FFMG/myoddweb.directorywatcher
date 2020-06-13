using NUnit.Framework;
using System.IO;
using System.Threading;
using System.Threading.Tasks;

namespace myoddweb.directorywatcher.test
{
  [TestFixture]
  internal class WatcherAddTests
  {
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
      using var helper = new HelperTest();

      // the number of files we will be adding
      var numberAdded = 0;

      // first we need to create the watcher
      using ( var watcher = new Watcher())
      {
        watcher.Add(new Request(helper.Folder, recursive));
        watcher.OnAddedAsync += ( fse,  token) =>
          {
            TestContext.Out.WriteLine( $"Adding {(fse.IsFile?"File":"Folder")}");
            if (fse.IsFile)
            {
              Interlocked.Increment( ref numberAdded );
            }
            return Task.CompletedTask;
          };
        watcher.Start();

        //we then need to wait a bit for all the workers to have started.
        TestContext.Out.WriteLine("Waiting for watchers!");

        //we then need to wait a bit for all the workers to have started.
        var timeout = (numberToAdd <= 2 ? 3 : numberToAdd) * 1000;
        Assert.IsTrue(SpinWait.SpinUntil(() => watcher.Ready(), timeout));

        TestContext.Out.WriteLine("All watchers ready!");

        for (var i = 0; i < numberToAdd; ++i)
        {
          // create an empty file
          helper.AddFile();
          await Task.Yield();
        }

        // give a bit of time
        SpinWait.SpinUntil( () => numberAdded >= numberToAdd, timeout );
      }
      Assert.AreEqual(numberToAdd, numberAdded);
    }
  }
}
