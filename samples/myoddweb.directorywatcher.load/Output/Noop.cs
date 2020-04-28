using System;
using System.Threading;

namespace myoddweb.directorywatcher.load.Output
{
  public sealed class Noop : IOutput
  {
    public void AddMessage(string message, CancellationToken token)
    {
      // do nothing
    }

    public void AddInformationMessage(DateTime dt, string message, CancellationToken token)
    {
      // do nothing
    }

    public void AddWarningMessage(DateTime dt, string message, CancellationToken token)
    {
      // do nothing
    }

    public void AddErrorMessage(DateTime dt, string message, CancellationToken token)
    {
      // do nothing
    }

    public void Dispose()
    {
      // do nothing
    }
  }
}
