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

    public static T LoadTypeFromAssembly<T>(Assembly assembly) where T : class
    {
      Type[] exportedTypes = assembly.GetExportedTypes();
      foreach (Type t in exportedTypes)
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
            if (e.InnerException is DllNotFoundException)
            {
              var dllNotFoundException = (DllNotFoundException)e.InnerException;
              Debug.WriteLine("A DllNotFoundException was thrown during the attempt to create a type instance. Are you missing some DLL dependencies?");
              throw dllNotFoundException;
            }
            throw;
          }
        }
      }
      return null;
    }
  }
}
