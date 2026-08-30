using System;
using System.Runtime.InteropServices.ComTypes;
using NUnit.Framework;
using NUnit.Framework.Legacy;

namespace myoddweb.directorywatcher.test
{
  [TestFixture]
  internal class RatesTests
  {
    [TestCase(1000)]
    [TestCase(42)]
    [TestCase(200)]
    public void EventRateIsSaved( long rate )
    {
      var rates = new Rates(rate , 200);
      ClassicAssert.AreEqual( rate, rates.EventsMilliseconds );
    }

    [TestCase(1000)]
    [TestCase(42)]
    [TestCase(200)]
    public void StatisticsRateIsSaved(long rate)
    {
      var rates = new Rates(1000, rate);
      ClassicAssert.AreEqual(rate, rates.StatisticsMilliseconds);
    }

    [Test]
    public void StatisticsDefaultIsZero()
    {
      var rates = new Rates(1000);
      ClassicAssert.AreEqual(0, rates.StatisticsMilliseconds);
    }

    [Test]
    public void EventsRateCannotBeNegative()
    {
      Assert.Throws<ArgumentException>( () => _ = new Rates(-1, 1000) );
    }

    [Test]
    public void StatisticsRateCannotBeNegative()
    {
      Assert.Throws<ArgumentException>(() => _ = new Rates(1000, -1));
    }

    [Test]
    public void EventsRateCanBeZero()
    {
      Assert.DoesNotThrow(() => _ = new Rates(0, 1000));
    }

    [Test]
    public void StatisticsRateCanBeZero()
    {
      Assert.DoesNotThrow(() => _ = new Rates(1000, 0));
    }
  }
}
