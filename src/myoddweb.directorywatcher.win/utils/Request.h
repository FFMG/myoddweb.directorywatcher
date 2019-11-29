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
    protected:
      Request() :
        _path(nullptr),
        _recursive(false),
        _callback(nullptr),
        _callbackRateMs(0)
      {
      }

    public:
      Request(const wchar_t* path, const bool recursive, const EventCallback& callback, const long long callbackRateMs) :
        Request()
      {
        Assign(path, recursive, callback, callbackRateMs);
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
        if (_path == nullptr)
        {
          return;
        }
        delete[] _path;
        _path = nullptr;
      }

      void Assign(const Request& request )
      {
        if (this == &request)
        {
          return;
        }
        Assign( request._path, request._recursive, request._callback, request._callbackRateMs );
      }

      void Assign(const wchar_t* path, const bool recursive, const EventCallback& callback, const long long callbackRateMs)
      {
        // clean up
        CleanPath();

        _callback = callback;
        _callbackRateMs = callbackRateMs;
        _recursive = recursive;

        if (path != nullptr)
        {
          auto l = wcslen(path);
          _path = new wchar_t[ l+1];
          wmemset(_path, L'\0', l+1);
          wcscpy_s(_path, l+1, path );
        }        
      }

    public:
      const wchar_t* Path() const {
        return _path;
      }

      const bool Recursive() const {
        return _recursive;
      }

      const EventCallback& Callback() const {
        return _callback;
      }

      long long CallbackRateMs() const {
        return _callbackRateMs;
      }

    private:
      /**
       * \brief the path of the folder we will be monitoring
       */
      wchar_t* _path;

      /**
       * \brief if we are recursively monitoring or not.
       */
      bool _recursive;

      /**
       * \brief the callback even we want to call from time to time.
       */
      EventCallback _callback;
      
      /**
       * How often we wish to callback
       */
      long long _callbackRateMs;
    };
  }
}
