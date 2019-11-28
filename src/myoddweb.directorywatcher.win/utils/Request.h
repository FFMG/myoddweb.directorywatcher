// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include <string>
#include "../monitors/Callbacks.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    /**
     * \brief unmannaged implementation of IRequest
     */
    class Request
    {
    public:
      Request() :
        Path(nullptr), 
        Recursive(false),
        Callback(nullptr),
        CallbackRateMs(0)
      {

      }

      Request( const wchar_t* path, const bool recursive, const EventCallback& callback, const long long callbackRateMs) :
        Request()
      {
        Assign(path, recursive, callback, CallbackRateMs );
      }

      Request(const Request& request) :
        Request()
      {
        Assign(request);
      }

      ~Request()
      {
        CleanPath();
      }

      // prevent assignment
      const Request& operator=(const Request& request) = delete;

    private :
      void CleanPath()
      {
        if (Path == nullptr)
        {
          return;
        }
        delete[] Path;
        Path = nullptr;
      }

      void Assign(const Request& request )
      {
        if (this == &request)
        {
          return;
        }
        Assign( request.Path, request.Recursive, request.Callback, request.CallbackRateMs );
      }

      void Assign(const wchar_t* path, const bool recursive, const EventCallback& callback, const long long callbackRateMs)
      {
        // clean up
        CleanPath();

        Callback = callback;
        CallbackRateMs = callbackRateMs;
        Recursive = recursive;

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

      /**
       * \brief the callback even we want to call from time to time.
       */
      EventCallback Callback; 
      
      /**
       * How often we wish to callback
       */
      long long CallbackRateMs;
    };
  }
}
