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
            TestContext.Out.WriteLine("Message to write to log");
            if (fse.IsFile)
            {
              ++numberAdded;
            }
            return Task.CompletedTask;
          };
        watcher.Start();

        //we then need to wait a bit for all the workers to have started.
        SpinWait.SpinUntil(() => watcher.Ready() );

        for (var i = 0; i < numberToAdd; ++i)
        {
          // create an empty file
          helper.AddFile();
          await Task.Yield();
        }

        // give a bit of time
        SpinWait.SpinUntil( () => numberAdded >= numberToAdd, numberToAdd * 1000 );
      }
      Assert.AreEqual(numberToAdd, numberAdded);
    }
  }
}
