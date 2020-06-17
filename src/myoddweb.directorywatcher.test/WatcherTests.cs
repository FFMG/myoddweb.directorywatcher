using myoddweb.directorywatcher.interfaces;
using NUnit.Framework;
using System.Diagnostics;
using System.IO;
using System.Threading;
using System.Threading.Tasks;

namespace myoddweb.directorywatcher.test
{
  [TestFixture]
  internal class WatcherTests
  {
    [SetUp]
    public void Setup()
    {
    }

    [Test]
    public void DefaultDoesNotThrow()
    {
      Assert.DoesNotThrow(() =>
      {
        var _ = new Watcher();
      });
    }

    [Test]
    public void StopWithoutStart()
    {
      using var watcher = new Watcher();
      watcher.Add(new Request("%temp%", false));
      Assert.DoesNotThrow( () => watcher.Stop() );
    }

    [TestCase(1, true)]
    [TestCase(5, true)]
    [TestCase(42, true)]
    [TestCase(1, false)]
    [TestCase(5, false)]
    [TestCase(42, false)]
    public Task GetNotifications( int number, bool recursive )
    {
      using var helper = new HelperTest();

      using var watcher = new Watcher();
      watcher.Add(new Request( helper.Folder , recursive, new Rates(100)));
      
      var added = 0;
      watcher.OnAddedAsync += (ft, t) =>
       {
         Interlocked.Increment(ref added);
         return Task.CompletedTask;
       };

      // start 
      watcher.Start();

      //we then need to wait a bit for all the workers to have started.
      TestContext.Out.WriteLine("Waiting for watchers!");

      //we then need to wait a bit for all the workers to have started.
      var timeout = number * 1000;
      Assert.IsTrue(SpinWait.SpinUntil(() => watcher.Ready(), timeout));

      TestContext.Out.WriteLine("All watchers ready!");

      for ( var i = 0; i < number; ++i )
      {
        helper.AddFile();
      }

      // wait a bit
      SpinWait.SpinUntil(() => number == added, timeout );

      watcher.Stop();

      // stop
      Assert.AreEqual(number, added);
      return Task.CompletedTask;
    }

    [TestCase(1, true)]
    [TestCase(5, true)]
    [TestCase(42, true)]
    [TestCase(1, false)]
    [TestCase(5, false)]
    [TestCase(42, false)]
    public void GetNotificationsMultipleWatchers(int number, bool recursive)
    {
      using var helper = new HelperTest();

      using var watcher1 = new Watcher();
      watcher1.Add(new Request(helper.Folder, recursive, new Rates(500)));
      using var watcher2 = new Watcher();
      watcher2.Add(new Request(helper.Folder, recursive, new Rates(500)));

      var added1 = 0;
      var added2 = 0;
      watcher1.OnAddedAsync += (ft, t) =>
      {
        Interlocked.Increment(ref added1 );
        return Task.CompletedTask;
      };
      watcher2.OnAddedAsync += (ft, t) =>
      {
        Interlocked.Increment(ref added2);
        return Task.CompletedTask;
      };

      // start 
      watcher1.Start();
      watcher2.Start();

      TestContext.Out.WriteLine("Waiting for watchers!");

      //we then need to wait a bit for all the workers to have started.
      var timeout = number * 1000;
      Assert.IsTrue( SpinWait.SpinUntil(() => watcher1.Ready() && watcher2.Ready(), timeout ));

      TestContext.Out.WriteLine("All watchers ready!");

      for (var i = 0; i < number; ++i)
      {
        helper.AddFile();
      }

      // wait a bit
      SpinWait.SpinUntil(() => number == added1 && number == added2, timeout );

      watcher1.Stop();
      watcher2.Stop();

      // stop
      Assert.AreEqual(number, added1);
      Assert.AreEqual(number, added2);
    }

    [TestCase(5, true)]
    [TestCase(5, false)]
    [Retry(5)]
    public void RemoveAddedDelegateBeforeStop(int number, bool recursive )
    {
      var added = 0;
      Task Fn(IFileSystemEvent ft, CancellationToken token)
      {
        Interlocked.Increment(ref added );
        return Task.CompletedTask;
      }

      using var helper = new HelperTest();
      using var watcher = new Watcher();
      watcher.Add(new Request(helper.Folder, recursive, new Rates(50)));
      watcher.Start();

      SpinWait.SpinUntil(() => watcher.Ready());

      watcher.OnAddedAsync += Fn;

      for (var i = 0; i < number; ++i)
      {
        helper.AddFile();
      }

      // wait a bit
      Assert.IsTrue( SpinWait.SpinUntil(() => number == added, number * 1000) );

      // check that they all added.
      Assert.AreEqual(number, added);

      // stop watching
      watcher.OnAddedAsync -= Fn;

      // add some more
      for (var i = 0; i < number; ++i)
      {
        helper.AddFile();
      }

      Assert.AreEqual(number, added);

      watcher.Stop();
    }

    [TestCase(true)]
    [TestCase(false)]
    public async Task DeleteWatchedFolder( bool recursive )
    {
      using var helper = new HelperTest();
      
      // create a folder will be actually watch
      var folder = helper.AddFolder();
      await Task.Delay(100).ConfigureAwait(false);

      // start watching it.
      using var watcher = new Watcher();
      watcher.Add(new Request(folder, recursive));
      watcher.Start();

      //we then need to wait a bit for all the workers to have started.
      SpinWait.SpinUntil(() => watcher.Ready());

      // try and delete it
      Assert.DoesNotThrow(() => Directory.Delete(folder, true));

      watcher.Stop();
    }

    [Test]
    public async Task RenameWatchedFolder()
    {

    }
  }
}