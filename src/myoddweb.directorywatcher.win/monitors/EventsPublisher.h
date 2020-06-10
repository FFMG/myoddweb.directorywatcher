// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#pragma once
#include "Callbacks.h"

namespace myoddweb::directorywatcher
{
  class Monitor;
  class EventsPublisher
  {
    Monitor& _monitor;
    const long long _id;
    const EventCallback& _callback;
    const long long _delayTimeMilliseconds;
    float _elapsedTimeMilliseconds;

  public:
    explicit EventsPublisher(Monitor& monitor, long long id,  const EventCallback& callback, long long delayTimeMilliseconds);

    /**
     * \brief called at various intervals.
     * \param fElapsedTimeMilliseconds the number of ms since the last update
     */
    void Update(float fElapsedTimeMilliseconds);

  private:
    /**
     * \brief get the events.
     */
    void GetEvents() const;

    /**
     * \brief check if the time has now elapsed.
     * \param fElapsedTimeMilliseconds the number of ms since the last time we checked.
     * \return if the time has elapsed and we can continue.
     */
    bool HasElapsed(float fElapsedTimeMilliseconds);
  };
}
