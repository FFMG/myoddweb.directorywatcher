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

  protected:
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
    Request(const wchar_t* path, bool recursive, const LoggerCallback& loggerCallback, const EventCallback& eventsCallback, const StatisticsCallback& statisticsCallback, long long eventsCallbackRateMs, long long statisticsCallbackRateMs);

  public:
    /**
     * \brief copy constructor
     */
    Request(const Request& request);

    /**
     * \brief create from a parent request, (no callback)
     * \param path the path being watched.
     * \param recursive if the request is recursive or not.
     * \param eventsCallbackRateMs how long we want to keep our events for.
     * \param statisticsCallbackRateMs how long we want to keep stats data for.
     */
    Request(const wchar_t* path, bool recursive, long long eventsCallbackRateMs, long long statisticsCallbackRateMs);
    virtual ~Request();

    /**
     * \brief the assignment operator
     * \param src the value we are assigning
     */
    Request& operator=(const Request& src);

    // prevent move
    Request(Request&&) = delete;
    const Request&& operator=(Request&& ) = delete;

  private :
    /**
     * \brief clean up all the values and free memory
     */
    void Dispose();

    /**
     * \brief assin a request
     * \param request the request value we want to copy
     */
    void Assign(const Request& request);

    /**
     * \brief Assign the values directly
     * \param path the path we are looking at
     * \param recursive if we are looking at that folder only or not
     * \param loggerCallback where we will receive log messages
     * \param eventsCallback where we will receive events
     * \param statisticsCallback where the statistics are logged
     * \param eventsCallbackRateMs how fast we want messages published
     * \param statisticsCallbackRateMs how fast we want statistics to be published.
     */
    void Assign(const wchar_t* path, bool recursive, const LoggerCallback& loggerCallback, const EventCallback& eventsCallback, const StatisticsCallback& statisticsCallback, long long eventsCallbackRateMs, long long statisticsCallbackRateMs);

  public:
    /**
     * \brief return if we are using events or not
     */
    [[nodiscard]]
    bool IsUsingEvents() const;

    /**
     * \brief return if we are using statistics or not
     */
    [[nodiscard]]
    bool IsUsingStatistics() const;

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
     * \brief the logger callback function
     */
    [[nodiscard]]
    const LoggerCallback& CallbackLogger() const;

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
     *       As set in the Delegates.cs file
     *   public struct Request
     *   {
     *     ...
     *   }
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
    StatisticsCallback _statisticsCallback;

    /**
     * How often we wish to callback events
     */
    long long _eventsCallbackRateMs;

    /**
     * How often we wish to callback stats
     */
    long long _statisticsCallbackRateMs;

    /**
     * \brief the logger callback
     */ 
    LoggerCallback _loggerCallback;
  };
}
