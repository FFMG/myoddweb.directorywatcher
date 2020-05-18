using System;
using System.Threading;
using myoddweb.commandlineparser;
using myoddweb.commandlineparser.OutputFormatter;
using myoddweb.commandlineparser.Rules;

namespace myoddweb.directorywatcher.load
{
  /// <summary>
  /// Load testing to test various scenarios
  /// Use "myoddweb.directorywatcher.load -help for details"
  /// </summary>
  internal class Program
  {
    private static void Main(string[] args)
    {
      var arguments = new CommandlineParser(args, new CommandlineArgumentRules
      {
        new HelpCommandlineArgumentRule( new []{"help", "h"} ),
        new RequiredCommandlineArgumentRule( new []{ "iterations", "i"}, "The number of test itarations we wish to run.\n\rThis is the number of time we want to randomly stop/start watching a folder." ),
        new OptionalCommandlineArgumentRule( new []{ "folders", "f"}, "5", "The number of folders watched at once." ),
        new OptionalCommandlineArgumentRule( new []{ "change", "c"}, "10", "How often we want to change to another folder (in seconds)." ),
        new OptionalCommandlineArgumentRule( new []{ "unique", "u"}, "true", "If we want to use a unique watcher, shared, or use a watcher per directory." ),
        new OptionalCommandlineArgumentRule( new []{ "stats", "s"}, "true", "Display stats every 10 seconds." ),
        new OptionalCommandlineArgumentRule( new []{ "quiet", "q"}, "false", "Do not display the folder updates." ),
        new OptionalCommandlineArgumentRule( new []{ "drives", "d"}, "false", "Test all the drives only." ),
      });

      // check for help
      if (arguments.IsHelp() )
      {
        var help = new ConsoleOutputFormatter( arguments  );
        help.Write(  );
        return;
      }

      var options = new Options(arguments);
      ILoad load = null;

      if (options.Drives)
      {
        load = new Drives(options);
      }
      else
      {
        load = new Load(options);
      }

      load?.Start();

      WaitForCtrlC();

      load?.Stop();
      load?.Dispose();
    }

    private static void WaitForCtrlC()
    {
      var exitEvent = new ManualResetEvent(false);
      Console.CancelKeyPress += delegate (object sender, ConsoleCancelEventArgs e)
      {
        e.Cancel = true;
        Console.WriteLine("Stop detected.");
        exitEvent.Set();
      };
      exitEvent.WaitOne();
    }

  }
}
