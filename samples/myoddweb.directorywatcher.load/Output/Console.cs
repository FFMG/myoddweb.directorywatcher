using System;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;
using ConsoleColor = System.ConsoleColor;

namespace myoddweb.directorywatcher.load.Output
{
  public sealed class Console : IOutput
  {
    /// <summary>
    /// All the currently running tasks.
    /// </summary>
    private readonly List<Task> _tasks = new List<Task>();

    private readonly CancellationTokenSource _token = new CancellationTokenSource();

    /// <summary>
    /// The original console color
    /// </summary>
    private readonly ConsoleColor _consoleColor;

    /// <summary>
    /// We need a static lock so it is shared by all.
    /// </summary>
    private static readonly object Lock = new object();

    public Console()
    {
      _consoleColor = System.Console.ForegroundColor;
    }

    public void AddMessage( string message, CancellationToken token)
    {
      // ReSharper disable once InconsistentlySynchronizedField
      // the color is never updated
      AddMessage(_consoleColor, DateTime.UtcNow, message, token);
    }

    public void AddWarningMessage(DateTime dt, string message, CancellationToken token)
    {
      AddMessage(ConsoleColor.Yellow, dt, message, token);
    }

    public void AddInformationMessage(DateTime dt, string message, CancellationToken token)
    {
      AddMessage(ConsoleColor.Blue, dt, message, token);
    }

    public void AddErrorMessage(DateTime dt, string message, CancellationToken token)
    {
      AddMessage(ConsoleColor.Red, dt, message, token);
    }

    private void AddMessage(ConsoleColor color, DateTime dt, string message, CancellationToken token)
    {
      using (var linkedTokenSource = CancellationTokenSource.CreateLinkedTokenSource(token, _token.Token))
      {
        var task = Task.Run(() =>
        {
          lock (Lock)
          {
            try
            {
              System.Console.ForegroundColor = color;
              System.Console.WriteLine($"[{dt:HH:mm:ss.ffff}]:{message}");
            }
            catch (Exception e)
            {
              System.Console.ForegroundColor = ConsoleColor.Red;
              System.Console.WriteLine(e.Message);
            }
            finally
            {
              System.Console.ForegroundColor = _consoleColor;
            }
          }
        }, linkedTokenSource.Token );

        lock (_tasks)
        {
          _tasks.Add(task);
          _tasks.RemoveAll(t => t.IsCompleted);
        }
      }
    }

    public void Dispose()
    {
      lock (_tasks)
      {
        _token.Cancel();
        _tasks.RemoveAll(t => t.IsCompleted);
        Task.WaitAll(_tasks.ToArray());
      }
    }
  }
}
