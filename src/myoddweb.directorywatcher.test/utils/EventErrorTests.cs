// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
using System;
using myoddweb.directorywatcher.utils;
using NUnit.Framework;

namespace myoddweb.directorywatcher.test.utils
{
  internal class EventErrorTests
  {
    [Test]
    public void ValuesAreSaved()
    {
      var utc = DateTime.UtcNow;
      const interfaces.EventError code = interfaces.EventError.Aborted;
      var err = new EventError( code, utc );
      Assert.AreEqual( err.Code, code );
      Assert.AreEqual( err.DateTimeUtc, utc);
    }

    [Test]
    public void InvalidCode()
    {
      var utc = DateTime.UtcNow;
      Assert.Throws<ArgumentOutOfRangeException>(() =>
      {
        var _ = new EventError((interfaces.EventError) (-1), utc);
      });
    }

    [TestCase((int)interfaces.EventError.General, "General error")]
    [TestCase((int)interfaces.EventError.Memory, "Guarded risk of memory corruption")]
    [TestCase((int)interfaces.EventError.Overflow, "Guarded risk of memory overflow")]
    [TestCase((int)interfaces.EventError.Aborted, "Monitoring was aborted")]
    [TestCase((int)interfaces.EventError.CannotStart,"Unable to start monitoring")]
    [TestCase((int)interfaces.EventError.Access, "Unable to access the given file/folder")]
    [TestCase((int)interfaces.EventError.NoFileData, "The raised event did not have any valid file name")]
    [TestCase((int)interfaces.EventError.CannotStop,"There was an issue trying to stop the watcher(s)")]
    [TestCase((int)interfaces.EventError.None, "No Error")]
    public void CheckMessage( int code, string message)
    {
      var utc = DateTime.UtcNow;
      var err = new EventError((interfaces.EventError)(code), utc);
      Assert.AreEqual(err.Message, message);
    }

    [TestCase((int)interfaces.EventError.General, "General error")]
    [TestCase((int)interfaces.EventError.Memory, "Guarded risk of memory corruption")]
    public void CheckMessageMoreThanOnce(int code, string message)
    {
      var utc = DateTime.UtcNow;
      var err = new EventError((interfaces.EventError)(code), utc);
      Assert.AreEqual(err.Message, message);
      Assert.AreEqual(err.Message, message); // get it again...
    }

  }
}
