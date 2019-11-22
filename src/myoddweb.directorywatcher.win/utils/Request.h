// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include <string>

namespace myoddweb
{
  namespace directorywatcher
  {
    /**
     * \brief unmanaged implementation of IRequest
     */
    class Request
    {
    public:
      Request() :Path(nullptr), Recursive(false)
      {

      }

      Request( const wchar_t* path, bool recursive) :
        Request()
      {
        Assign(path, recursive);
      }


      Request(const Request& request ) : 
        Request()
      {
        Assign(request);
      }

      const Request& operator=(const Request& request)
      {
        Assign(request);
        return *this;
      }

      ~Request()
      {
        CleanPath();
      }

    private :
      void CleanPath()
      {
        if (Path != nullptr)
        {
          delete[] Path;
        }
        Path = nullptr;
      }

      void Assign(const Request& request )
      {
        if (this == &request)
        {
          return;
        }
        Assign(request.Path, request.Recursive);
      }

      void Assign(const wchar_t* path, bool recursive)
      {
        Recursive = recursive;
        CleanPath();
        Path = nullptr;
        if (path != nullptr)
        {
          auto l = wcslen(path);
          Path = new wchar_t[ l+1];
          wmemset(Path, L'\0', l+1);
          wcscpy_s(Path, l+1, path );
        }        
      }

    public:
      /**
       * \brief the path of the folder we will be monitoring
       */
      wchar_t* Path;

      /**
       * \brief if we are recursively monitoring or not.
       */
      bool Recursive;
    };
  }
}
