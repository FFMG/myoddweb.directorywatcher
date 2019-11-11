// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
using System;
using myoddweb.directorywatcher.interfaces;
using NUnit.Framework;

namespace myoddweb.directorywatcher.test
{
  internal class RequestTests
  {
    [Test]
    public void IsCorrectInterface()
    {
      var request = new Request( "C:\\", false );
      Assert.IsInstanceOf<IRequest>( request );
    }

    [TestCase("c:\\")]
    [TestCase("z:\\")]
    [TestCase("C:\\")]
    [TestCase("c:\\Full\\Path\\")]
    [TestCase("c:\\Full\\Path")]
    public void PathIsSaved( string path )
    {
      var request = new Request( path, false);
      Assert.AreEqual(path, request.Path);
    }

    [TestCase(true)]
    [TestCase(false)]
    public void RecursiveIsSaved(bool recursive)
    {
      var request = new Request("c:\\", recursive);
      Assert.AreEqual(recursive, request.Recursive);
    }

    [Test]
    public void CannotCreateWithNullPath()
    {
      Assert.Throws<ArgumentNullException>(() =>
      {
        var _ = new Request(null, false);
      });
    }
  }
}
