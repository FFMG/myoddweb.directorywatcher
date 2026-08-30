// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include "../utils/Request.h"

namespace myoddweb::directorywatcher
{
  struct event;
  class Monitor;
  class EventsPublisher
  {
    Monitor& _monitor;
    const long long _id;
    const Request& _request;
    float _elapsedEventsTimeMilliseconds;
    float _elapsedStatisticsTimeMilliseconds;

    struct current_statistics
    {
      long long numberOfEvents;
    };

    /**
     * \brief our current statistics
     */
    current_statistics _currentStatistics{};

  public:
    explicit EventsPublisher(Monitor& monitor, long long id, const Request& request );

    /**
     * \brief called at various intervals.
     * \param fElapsedTimeMilliseconds the number of ms since the last update
     */
    void update(float fElapsedTimeMilliseconds);

  private:
    /**
     * \brief called at various intervals.
     * \param fElapsedTimeMilliseconds the number of ms since the last update
     */
    void update_events(float fElapsedTimeMilliseconds);

    /**
     * \brief called at various intervals.
     * \param fElapsedTimeMilliseconds the number of ms since the last update
     */
    void update_statistics(float fElapsedTimeMilliseconds);

    /**
     * \brief get the events.
     * \param actualElapsedTimeMilliseconds the number of ms since the last time we published
     */
    void publish_statistics(float actualElapsedTimeMilliseconds);

    /**
     * \brief get the events.
     */
    void publish_events();

    /**
     * \brief update the stats with the given event
     * \paranm event the event we will update the stats with
     */
    void update_statistics(const event& event);

    /**
     * \brief check if the events time has now elapsed.
     * \param fElapsedTimeMilliseconds the number of ms since the last time we checked.
     * \return if the time has elapsed and we can continue.
     */
    bool has_events_elapsed(float fElapsedTimeMilliseconds);

    /**
     * \brief check if the statusticstime has now elapsed.
     * \param fElapsedTimeMilliseconds the number of ms since the last time we checked.
     * \return 0 if the number has not elapsed otherwise the number of ms elapsed
     */
    float has_statistics_elapsed(float fElapsedTimeMilliseconds);

    /**
     * \brief make sure that all the stats values are up to date
     *        if we are not collecting any information
     */
    void ensure_statistics_are_up_to_date_if_not_collecting_events();
  };
}
