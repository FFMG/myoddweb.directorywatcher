using NUnit.Framework;

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
  }
}