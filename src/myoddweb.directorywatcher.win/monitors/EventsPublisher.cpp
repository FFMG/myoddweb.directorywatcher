// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#include "EventsPublisher.h"
#include <vector>
#include "../utils/Event.h"
#include "../utils/Instrumentor.h"
#include "Monitor.h"

namespace myoddweb::directorywatcher
{
  EventsPublisher::EventsPublisher(Monitor& monitor, const long long id, const EventCallback& callback, const long long delayTimeMilliseconds)
    :
    _monitor( monitor),
    _id(id),
    _callback( callback),
    _delayTimeMilliseconds( delayTimeMilliseconds),
    _elapsedTimeMilliseconds(0)
  {

  }

  void EventsPublisher::Update(const float fElapsedTimeMilliseconds)
  {
    // check if we are ready.
    _elapsedTimeMilliseconds += fElapsedTimeMilliseconds;
    if (_elapsedTimeMilliseconds < static_cast<float>(_delayTimeMilliseconds))
    {
      return;
    }

    //  restart the timer.
    _elapsedTimeMilliseconds -= static_cast<float>(_delayTimeMilliseconds);

    // get the events
    GetEvents();
  }

  /**
   * \brief get all the events and send them over to the callback.
   */
  void EventsPublisher::GetEvents() const
  {
    MYODDWEB_PROFILE_FUNCTION();

    // get the events.
    auto events = std::vector<Event*>();
    if (0 == _monitor.GetEvents(events))
    {
      return;
    }

    // then call the callback
    for (auto it = events.begin(); it != events.end(); ++it)
    {
      const auto& event = (*it);
      try
      {
        _callback(
          _id,
          event->IsFile,
          event->Name,
          event->OldName,
          event->Action,
          event->Error,
          event->TimeMillisecondsUtc
          );
      }
      catch (...)
      {
        // the callback did something wrong!
        // we should log it somewhere.
      }

      // we are done with the event
      // so we can get rid of it.
      delete event;
    }
  }
}
