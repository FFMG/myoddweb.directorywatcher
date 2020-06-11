using myoddweb.directorywatcher.interfaces;
using myoddweb.directorywatcher.utils.Helper;
using System;
using System.Diagnostics.Contracts;

namespace myoddweb.directorywatcher.utils
{
  internal class WatcherManagerEmbeddedLoadLibrary : WatcherManagerCommon
  {
    #region Member variables
    /// <summary>
    /// The Native dll helper
    /// </summary>    
    private readonly WatcherManagerNativeLibrary _helper;
    #endregion

    public WatcherManagerEmbeddedLoadLibrary()
    {
      // Create helper we will throw if the file does not exist.
      _helper = new WatcherManagerNativeLibrary(GetFromEmbedded(), EventsCallback, StatisticsCallback);
    }

    #region Private Methods
    private string GetFromEmbedded()
    {
      return Environment.Is64BitProcess ? GetFromEmbeddedx64() : GetFromEmbeddedx86();
    }

    private string GetFromEmbeddedx86()
    {
      Contract.Assert(!Environment.Is64BitProcess);
      return CreateResourceFile("x86.directorywatcher.win", "myoddweb.directorywatcher.win.x86.dll");
    }

    private string GetFromEmbeddedx64()
    {
      Contract.Assert(Environment.Is64BitProcess);
      return CreateResourceFile("x64.directorywatcher.win", "myoddweb.directorywatcher.win.x64.dll");
    }
    #endregion

    #region Abstract methods
    public override long Start(IRequest request)
    {
      return _helper.Start(request);
    }

    public override bool Stop(long id)
    {
      return _helper.Stop(id);
    }

    public override bool Ready()
    {
      return _helper.Ready();
    }
    #endregion
  }
}
