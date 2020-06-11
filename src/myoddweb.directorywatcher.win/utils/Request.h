// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include "../monitors/Callbacks.h"

namespace myoddweb:: directorywatcher
{
  /**
   * \brief unmannaged implementation of IRequest
   */
  class Request
  {
    /**
     * \brief private constructor prevent empty requests.
     */
    Request();

  public:
    /**
     * \brief copy constructor
     */
    Request(const Request& request);

    /**
     * \brief create from a parent request, (no callback)
     * \param path the path being watched.
     * \param recursive if the request is recursive or not.
     */
    Request(const wchar_t* path, bool recursive);
    ~Request();

    // prevent assignment + move
    Request(Request&&) = delete;
    const Request& operator=(const Request& ) = delete;
    const Request&& operator=(Request&& ) = delete;

  private :
    /**
     * \brief Dispose all the values.
     */
    void Dispose();

    /**
     * \brief assin a request
     */
    void Assign(const Request& request);

    /**
     * \brief assign request values
     */
    void Assign(const wchar_t* path, bool recursive, const EventCallback& eventsCallback, const StatisticsCallback& statisticsCallback, long long eventsCallbackRateMs, long long statisticsCallbackRateMs);

  public:
    /**
     * \brief access the path
     */
    [[nodiscard]]
    const wchar_t* Path() const;

    /**
     * \breif check if the request is recursive or not
     */
    [[nodiscard]]
    bool Recursive() const;

    /**
     * \brief the events callback
     * \return the event callback
     */
    [[nodiscard]]
    const EventCallback& CallbackEvents() const;

    /**
     * \brief the stats of the monitor 
     */
    [[nodiscard]]
    const StatisticsCallback& CallbackStatistics() const;

    /**
     * \brief how often we want to check for callbacks
     */
    [[nodiscard]]
    long long EventsCallbackRateMilliseconds() const;

    /**
     * \brief how often we want to check for stats
     */
    [[nodiscard]]
    long long StatsCallbackRateMilliseconds() const;

  private:

    /**
     *   NB: THE ORDER OF THE VARIABLES IS IMPORTANT!
     */


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
    EventCallback _eventsCallback;

    /**
     * \brief the callback even we want to call from time to time.
     */
    StatisticsCallback _statsCallback;

    /**
     * How often we wish to callback events
     */
    long long _eventsCallbackRateMs;

    /**
     * How often we wish to callback stats
     */
    long long _statisticsCallbackRateMs;
  };
}
