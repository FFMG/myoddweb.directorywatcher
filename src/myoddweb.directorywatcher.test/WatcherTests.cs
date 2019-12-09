using myoddweb.directorywatcher.interfaces;
using NUnit.Framework;
using System.Diagnostics;
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
      watcher.Add(new Request( helper.Folder , recursive));
      
      var added = 0;
      watcher.OnAddedAsync += (ft, t) =>
       {
         ++added;
         return Task.CompletedTask;
       };

      // start 
      watcher.Start();
      for( var i = 0; i < number; ++i )
      {
        helper.AddFile();
      }

      // wait a bit
      _ = SpinWait.SpinUntil(() =>
        {
          return number == added;
        }, number * 1000);

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
      watcher1.Add(new Request(helper.Folder, recursive));
      using var watcher2 = new Watcher();
      watcher2.Add(new Request(helper.Folder, recursive));

      var added1 = 0;
      var added2 = 0;
      watcher1.OnAddedAsync += (ft, t) =>
      {
        ++added1;
        return Task.CompletedTask;
      };
      watcher2.OnAddedAsync += (ft, t) =>
      {
        ++added2;
        return Task.CompletedTask;
      };

      // start 
      watcher1.Start();
      watcher2.Start();

      for (var i = 0; i < number; ++i)
      {
        helper.AddFile();
      }

      // wait a bit
      _ = SpinWait.SpinUntil(() =>
      {
        return number == added1 && number == added2;
      }, number * 1000);

      watcher1.Stop();
      watcher2.Stop();

      // stop
      Assert.AreEqual(number, added1);
      Assert.AreEqual(number, added2);
    }

    [TestCase(5, true)]
    [TestCase(5, false)]
    public async Task RemoveDelegateBeforeStop(int number, bool recursive )
    {
      var added = 0;
      Task fn(IFileSystemEvent ft, CancellationToken token)
      {
        ++added;
        return Task.CompletedTask;
      };

      using var helper = new HelperTest();
      using var watcher = new Watcher();
      watcher.Add(new Request(helper.Folder, recursive));
      watcher.Start();

      watcher.OnAddedAsync += fn;

      for (var i = 0; i < number; ++i)
      {
        helper.AddFile();
      }

      // wait a bit
      var stopWatch = new Stopwatch();
      stopWatch.Start();
      _ = SpinWait.SpinUntil(() =>
      {
        return number == added;
      }, number * 1000);
      stopWatch.Stop();

      // check that they all added.
      Assert.AreEqual(number, added);

      // stop watching
      watcher.OnAddedAsync -= fn;

      // add some more
      for (var i = 0; i < number; ++i)
      {
        helper.AddFile();
      }

      await Task.Delay((int)stopWatch.ElapsedMilliseconds).ConfigureAwait( false );
      Assert.AreEqual(number, added);

      watcher.Stop();
    }
  }
}