//This file is part of Myoddweb.Directorywatcher.
//
//    Myoddweb.Directorywatcher is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    Myoddweb.Directorywatcher is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Myoddweb.Directorywatcher.  If not, see<https://www.gnu.org/licenses/gpl-3.0.en.html>.
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
