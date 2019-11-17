// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
using System;
using System.Diagnostics;
using System.Reflection;

namespace myoddweb.directorywatcher.utils
{
  internal static class TypeLoader
  {
    public static T LoadTypeFromAssembly<T>(string assemblyFilePath) where T : class
    {
      var assembly = Assembly.LoadFrom(assemblyFilePath);
      return LoadTypeFromAssembly<T>(assembly);
    }

    private static T LoadTypeFromAssembly<T>(Assembly assembly) where T : class
    {
      var exportedTypes = assembly.GetExportedTypes();
      foreach (var t in exportedTypes)
      {
        // When coming from different assemblies the types don't match, this is very fragile.
        //if( typeof( T ).IsAssignableFrom( t ) )
        if (t.GetInterface(typeof(T).FullName, true) != null)
        {
          try
          {
            Debug.WriteLine("Trying to create instance of {0}...", t.FullName);
            return assembly.CreateInstance(t.FullName) as T;
          }
          catch (TargetInvocationException e)
          {
            if (!(e.InnerException is DllNotFoundException dllNotFoundException))
            {
              throw;

            }
            Debug.WriteLine("A DllNotFoundException was thrown during the attempt to create a type instance. Are you missing some DLL dependencies?");
            throw dllNotFoundException;
          }
        }
      }
      return null;
    }
  }
}
