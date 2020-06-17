using System;
using myoddweb.directorywatcher.interfaces;
using myoddweb.directorywatcher.utils.Helper;
using NUnit.Framework;

namespace myoddweb.directorywatcher.test
{
  [TestFixture]
  internal class LoggerEventTest
  {
    [TestCase(12)]
    [TestCase(400)]
    [TestCase(long.MaxValue)]
    public void IdIsSaved(long id)
    {
      var loggerEvent = new LoggerEvent(id, LogLevel.Unknown, "Blah");
      Assert.AreEqual( id, loggerEvent.Id );
    }

    [Test]
    public void IdCanBeZero()
    {
      var loggerEvent = new LoggerEvent(0, LogLevel.Unknown, "Blah");
      Assert.AreEqual(0, loggerEvent.Id);
    }

    [TestCase( LogLevel.Debug )]
    [TestCase(LogLevel.Error)]
    [TestCase(LogLevel.Information)]
    [TestCase(LogLevel.Panic)]
    [TestCase(LogLevel.Unknown)]
    [TestCase(LogLevel.Warning)]
    public void LogLevelIsSaved( LogLevel ll )
    {
      var loggerEvent = new LoggerEvent(42, ll, "Blah");
      Assert.AreEqual(ll, loggerEvent.LogLevel);
    }

    [Test]
    public void MessageCannotBeNull()
    {
      Assert.Throws<ArgumentNullException>( () => _ = new LoggerEvent(0, LogLevel.Unknown, null));
    }
  }
}
