// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include "../utils/Request.h"

namespace myoddweb::directorywatcher
{
  class Event;
  class Monitor;
  class EventsPublisher
  {
    Monitor& _monitor;
    const long long _id;
    const Request& _request;
    float _elapsedEventsTimeMilliseconds;
    float _elapsedStatisticsTimeMilliseconds;

    struct CurrentStatistics
    {
      long long numberOfEvents;
    };

    /**
     * \brief our current statistics
     */
    CurrentStatistics _currentStatistics{};

  public:
    explicit EventsPublisher(Monitor& monitor, long long id, const Request& request );

    /**
     * \brief called at various intervals.
     * \param fElapsedTimeMilliseconds the number of ms since the last update
     */
    void Update(float fElapsedTimeMilliseconds);

  private:
    /**
     * \brief called at various intervals.
     * \param fElapsedTimeMilliseconds the number of ms since the last update
     */
    void UpdateEvents(float fElapsedTimeMilliseconds);

    /**
     * \brief called at various intervals.
     * \param fElapsedTimeMilliseconds the number of ms since the last update
     */
    void UpdateStatistics(float fElapsedTimeMilliseconds);

    /**
     * \brief get the events.
     * \param actualElapsedTimeMilliseconds the number of ms since the last time we published
     */
    void PublishStatistics(float actualElapsedTimeMilliseconds);

    /**
     * \brief get the events.
     */
    void PublishEvents();

    /**
     * \brief update the stats with the given event
     * \paranm event the event we will update the stats with
     */
    void UpdateStatistics(const Event& event);

    /**
     * \brief check if the events time has now elapsed.
     * \param fElapsedTimeMilliseconds the number of ms since the last time we checked.
     * \return if the time has elapsed and we can continue.
     */
    bool HasEventsElapsed(float fElapsedTimeMilliseconds);

    /**
     * \brief check if the statusticstime has now elapsed.
     * \param fElapsedTimeMilliseconds the number of ms since the last time we checked.
     * \return 0 if the number has not elapsed otherwise the number of ms elapsed
     */
    float HasStatisticsElapsed(float fElapsedTimeMilliseconds);

    /**
     * \brief return if we are using events or not
     */
    bool IsUsingEvents() const;

    /**
     * \brief return if we are using statistics or not
     */
    bool IsUsingStatistics() const;
  };
}
