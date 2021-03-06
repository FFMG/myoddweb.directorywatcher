﻿using NUnit.Framework;
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

    [TestCase(true)]
    [TestCase(false)]
    public async Task StartWatcherBeforeAddingAnything( bool recursive)
    {
      using var helper = new HelperTest();
      using var watcher = new Watcher();

      // start the watcher
      watcher.Start();

      // the number of files we actually added
      var numberAdded = 0;
      const int numberToAdd = 10;
      var timeout = (numberToAdd <= 2 ? 3 : numberToAdd) * 1000;
      Assert.IsTrue(SpinWait.SpinUntil(() => watcher.Ready(), timeout));

      // and then add the folder we are after
      watcher.Add(new Request(helper.Folder, recursive));

      // then wait
      Assert.IsTrue(SpinWait.SpinUntil(() => watcher.Ready(), timeout));

      watcher.OnAddedAsync += (fse, token) =>
      {
        Interlocked.Increment(ref numberAdded);
        return Task.CompletedTask;
      };

      TestContext.Out.WriteLine("All watchers ready!");

      for (var i = 0; i < numberToAdd; ++i)
      {
        // create an empty file
        helper.AddFile();
        await Task.Yield();
      }

      // give a bit of time
      SpinWait.SpinUntil(() => numberAdded >= numberToAdd, timeout);
    }

    [Test]
    public void IfWeHaveNoFoldersToWatchThenStartReturnsFalse()
    {
      using var helper = new HelperTest();
      using var watcher = new Watcher();

      // start the watcher, we return false as we have not folders.
      Assert.IsFalse(watcher.Start());

      // stop it.
      watcher.Stop();
    }

    [Test]
    public void StartedWatcherWithNoFolderIsAlwaysReady()
    {
      using var helper = new HelperTest();
      using var watcher = new Watcher();

      // start the watcher
      watcher.Start();

      // nothing was added, but we are ready
      Assert.IsTrue(watcher.Ready());

      // stop it.
      watcher.Stop();
    }
  }

}