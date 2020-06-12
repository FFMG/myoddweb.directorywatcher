// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#include <string>
#include "Request.h"

namespace myoddweb:: directorywatcher
{
  /**
   * \brief unmannaged implementation of IRequest
   */
  Request::Request() :
    _path(nullptr),
    _recursive(false),
    _eventsCallback(nullptr),
    _statsCallback(nullptr),
    _eventsCallbackRateMs(0),
    _statisticsCallbackRateMs(0)
  {
  }

  /**
   * \brief create from a parent request, (no callback)
   * \param path the path being watched.
   * \param recursive if the request is recursive or not.
   */
  Request::Request(const wchar_t* path, const bool recursive) :
    Request()
  {
    Assign(path, recursive, nullptr, nullptr, 0, 0 );
  }
    
  Request::Request(const Request& request) :
    Request()
  {
    Assign(request);
  }

  /**
   * \brief Dispose all the values.
   */
  Request::~Request()
  {
    Dispose();
  }

  void Request::Dispose()
  {
    if (_path == nullptr)
    {
      return;
    }
    delete[] _path;
    _path = nullptr;
  }

  /**
   * \brief assin a request
   */
  void Request::Assign(const Request& request )
  {
    if (this == &request)
    {
      return;
    }
    Assign( request._path, request._recursive, request._eventsCallback, request._statsCallback, request._eventsCallbackRateMs, request._statisticsCallbackRateMs );
  }

  /**
   * \brief assign request values
   */
  void Request::Assign(
    const wchar_t* path, 
    const bool recursive, 
    const EventCallback& eventsCallback,
    const StatisticsCallback& statisticsCallback,
    const long long eventsCallbackRateMs,
    const long long statisticsCallbackRateMs)
  {
    // clean up
    Dispose();

    _eventsCallback = eventsCallback;
    _eventsCallbackRateMs = eventsCallbackRateMs;
    _statsCallback = statisticsCallback;
    _statisticsCallbackRateMs = statisticsCallbackRateMs;
    _recursive = recursive;

    if (path != nullptr)
    {
      auto l = wcslen(path);
      _path = new wchar_t[ l+1];
      wmemset(_path, L'\0', l+1);
      wcscpy_s(_path, l+1, path );
    }        
  }

  /**
   * \brief access the path
   */
  [[nodiscard]]
  const wchar_t* Request::Path() const
  {
    return _path;
  }

  /**
   * \breif check if the request is recursive or not
   */
  [[nodiscard]]
  bool Request::Recursive() const
  {
    return _recursive;
  }

  /**
   * \brief the events callback
   * \return the event callback
   */
  [[nodiscard]]
  const EventCallback& Request::CallbackEvents() const
  {
    return _eventsCallback;
  }

  /**
   * \brief the stats of the monitor
   */
  [[nodiscard]]
  const StatisticsCallback& Request::CallbackStatistics() const
  {
    return _statsCallback;
  }

  /**
   * \brief how often we want to check for callbacks
   */
  [[nodiscard]]
  long long Request::EventsCallbackRateMilliseconds() const
  {
    return _eventsCallbackRateMs;
  }

  /**
   * \brief how often we want to check for stats
   */
  [[nodiscard]]
  long long Request::StatsCallbackRateMilliseconds() const
  {
    return _statisticsCallbackRateMs;
  }

  /**
   * \brief return if we are using events or not
   */
  bool Request::IsUsingEvents() const
  {
    // null is allowed
    if (nullptr == CallbackEvents())
    {
      return false;
    }

    // zero are allowed.
    if (0 == EventsCallbackRateMilliseconds())
    {
      return false;
    }
    return true;
  }

  /**
   * \brief return if we are using statistics or not
   */
  bool Request::IsUsingStatistics() const
  {
    // null is allowed
    if (nullptr == CallbackStatistics())
    {
      return false;
    }

    // zero are allowed.
    if (0 == StatsCallbackRateMilliseconds())
    {
      return false;
    }

    // we are using it
    return true;
  }
}