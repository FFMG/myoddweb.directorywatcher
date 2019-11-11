using NUnit.Framework;

namespace myoddweb.directorywatcher.test
{
  internal class WatcherTests
  {
    [SetUp]
    public void Setup()
    {
    }

    [Test]
    public void StopWithoutStart()
    {
      using var watcher = new Watcher();
      watcher.Add(new Request("%tem%", false));
      Assert.DoesNotThrow( () => watcher.Stop() );
    }
  }
}