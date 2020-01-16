using System;
using System.Collections.Generic;
using myoddweb.commandlineparser;

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
      var arguments = new CommandlineParser(args, new Dictionary<string, CommandlineData>
      {
        {"help", new CommandlineData{ IsRequired = false } }
      });

      // check for help
      if (arguments.IsSet("help"))
      {
        DisplayHelp();
        return;
      }
    }

    private static void DisplayHelp()
    {
      Console.WriteLine("           myoddweb.directorywatcher.load");
      Console.WriteLine("           ------------------------------");
      Console.WriteLine("help............... Display this message." );
    }
  }
}
