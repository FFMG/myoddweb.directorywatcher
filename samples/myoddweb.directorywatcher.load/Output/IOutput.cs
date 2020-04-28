using System;
using System.Threading;
namespace myoddweb.directorywatcher.load.Output
{
  internal interface IOutput : IDisposable
  {
    void AddMessage(string message, CancellationToken token);

    void AddInformationMessage(DateTime dt, string message, CancellationToken token);

    void AddWarningMessage( DateTime dt, string message, CancellationToken token);

    void AddErrorMessage( DateTime dt, string message, CancellationToken token);
  }
}
