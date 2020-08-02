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
    _statisticsCallback(nullptr),
    _eventsCallbackRateMs(0),
    _statisticsCallbackRateMs(0),
    _loggerCallback(nullptr)
  {
  }

  /**
   * \brief protected constructor for unit tests.
   * \param path the path we are looking at
   * \param recursive if we are looking at that folder only or not
   * \param loggerCallback where we will receive log messages
   * \param eventsCallback where we will receive events
   * \param statisticsCallback where the statistics are logged
   * \param eventsCallbackRateMs how fast we want messages published
   * \param statisticsCallbackRateMs how fast we want statistics to be published.
   */
  Request::Request(
    const wchar_t* path, 
    const bool recursive, 
    const LoggerCallback& loggerCallback, 
    const EventCallback& eventsCallback, 
    const StatisticsCallback& statisticsCallback, 
    const long long eventsCallbackRateMs,
    const long long statisticsCallbackRateMs) :
    Request()
  {
    Assign(path, recursive, loggerCallback, eventsCallback, statisticsCallback, eventsCallbackRateMs, statisticsCallbackRateMs);
  }

  /**
   * \brief the assignment operator
   * \param src the value we are assigning
   */
  Request& Request::operator=(const Request& src )
  {
    if( this != &src )
    {
      Assign(src);
    }
    return *this;
  }

  /**
   * \brief create from a parent request, (no callback)
   * \param path the path being watched.
   * \param recursive if the request is recursive or not.
   * \param eventsCallbackRateMs how long we want to keep our events for.
   * \param statisticsCallbackRateMs how long we want to keep stats data for.
   */
  Request::Request(const wchar_t* path, bool recursive, const long long eventsCallbackRateMs, const long long statisticsCallbackRateMs) :
    Request()
  {
    Assign(path, recursive, nullptr, nullptr, nullptr, eventsCallbackRateMs, statisticsCallbackRateMs);
  }

  Request::Request(const sRequest& request) :
    Request()
  {
    Assign(
      request.Path, 
      request.Recursive, 
      request.LoggerCallback, 
      request.EventsCallback, 
      request.StatisticsCallback, 
      request.EventsCallbackRateMs, 
      request.StatisticsCallbackRateMs);
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

  /**
   * \brief clean up all the values and free memory
   */
  void Request::Dispose()
  {
    _loggerCallback = nullptr;
    _eventsCallback = nullptr;
    _statisticsCallback = nullptr;
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
    Assign( request._path, request._recursive, request._loggerCallback, request._eventsCallback, request._statisticsCallback, request._eventsCallbackRateMs, request._statisticsCallbackRateMs );
  }

  /**
   * \brief assign request values
   */
  void Request::Assign(
    const wchar_t* path, 
    const bool recursive,
    const LoggerCallback& loggerCallback,
    const EventCallback& eventsCallback,
    const StatisticsCallback& statisticsCallback,
    const long long eventsCallbackRateMs,
    const long long statisticsCallbackRateMs)
  {
    // clean up
    Dispose();

    _loggerCallback = loggerCallback;
    _eventsCallback = eventsCallback;
    _eventsCallbackRateMs = eventsCallbackRateMs;
    _statisticsCallback = statisticsCallback;
    _statisticsCallbackRateMs = statisticsCallbackRateMs;
    _recursive = recursive;

    if (path != nullptr)
    {
      const auto l = wcslen(path);
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
    return _statisticsCallback;
  }

  /**
   * \brief the logger callback
   */
  [[nodiscard]]
  const LoggerCallback& Request::CallbackLogger() const
  {
    return _loggerCallback;
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