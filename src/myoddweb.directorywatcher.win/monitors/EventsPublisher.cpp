// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#include "EventsPublisher.h"
#include <vector>
#include "../utils/Event.h"
#include "../utils/Instrumentor.h"
#include "../utils/Logger.h"
#include "../utils/LogLevel.h"
#include "Monitor.h"

namespace myoddweb::directorywatcher
{
  EventsPublisher::EventsPublisher(Monitor& monitor, const long long id, const Request& request)
    :
    _monitor(monitor),
    _id(id),
    _request(request),
    _elapsedEventsTimeMilliseconds(0),
    _elapsedStatisticsTimeMilliseconds(0)
  {

  }

  /**
   * \brief check if the time has now elapsed.
   * \param fElapsedTimeMilliseconds the number of ms since the last time we checked.
   * \return if the time has elapsed and we can continue.
   */
  bool EventsPublisher::has_events_elapsed(const float fElapsedTimeMilliseconds)
  {
    if( !_request.is_using_events())
    {
      return false;
    }

    _elapsedEventsTimeMilliseconds += fElapsedTimeMilliseconds;
    if (_elapsedEventsTimeMilliseconds < static_cast<float>(_request.events_callback_rate_milliseconds()))
    {
      return false;
    }

    //  restart the timer.
    while (_elapsedEventsTimeMilliseconds > static_cast<float>(_request.events_callback_rate_milliseconds())) {
      _elapsedEventsTimeMilliseconds -= static_cast<float>(_request.events_callback_rate_milliseconds());
    }
    return true;
  }

  float EventsPublisher::has_statistics_elapsed(const float fElapsedTimeMilliseconds)
  {
    // are we using stats?
    if( !_request.is_using_statistics())
    {
      return 0;
    }

    _elapsedStatisticsTimeMilliseconds += fElapsedTimeMilliseconds;
    if (_elapsedStatisticsTimeMilliseconds < static_cast<float>(_request.stats_callback_rate_milliseconds()))
    {
      return 0;
    }

    const auto actualElapsedTimeMilliseconds = _elapsedStatisticsTimeMilliseconds;

    //  restart the timer.
    while (_elapsedStatisticsTimeMilliseconds > static_cast<float>(_request.stats_callback_rate_milliseconds())) {
      _elapsedStatisticsTimeMilliseconds -= static_cast<float>(_request.stats_callback_rate_milliseconds());
    }
    return actualElapsedTimeMilliseconds;
  }

  void EventsPublisher::update(const float fElapsedTimeMilliseconds)
  {
    // first check the events
    update_events(fElapsedTimeMilliseconds);

    // then the stats
    update_statistics(fElapsedTimeMilliseconds);
  }

  /**
   * \brief called at various intervals.
   * \param fElapsedTimeMilliseconds the number of ms since the last update
   */
  void EventsPublisher::update_events(float fElapsedTimeMilliseconds)
  {
    // check if we are ready.
    if (!has_events_elapsed(fElapsedTimeMilliseconds))
    {
      return;
    }

    // get the events
    publish_events();
  }

  /**
   * \brief called at various intervals.
   * \param fElapsedTimeMilliseconds the number of ms since the last update
   */
  void EventsPublisher::update_statistics(float fElapsedTimeMilliseconds)
  {
    // check if we are ready.
    const auto actualElapsedTimeMilliseconds = has_statistics_elapsed(fElapsedTimeMilliseconds);
    if (actualElapsedTimeMilliseconds == 0 )
    {
      return;
    }

    // we need to double check that events are indeed supported
    // if not, then we need to do the updating ourselves.
    ensure_statistics_are_up_to_date_if_not_collecting_events();

    // then we can publish the stats
    publish_statistics(actualElapsedTimeMilliseconds);
  }

  /**
   * \brief make sure that all the stats values are up to date
   *        if we are not collecting any information
   */
  void EventsPublisher::ensure_statistics_are_up_to_date_if_not_collecting_events()
  {
    // if we are collecting events, then things should be fine
    // we cannot get events for the purpose of updating stats
    // otherwiswe could loose events.
    if (_request.is_using_events())
    {
      return;
    }

    // other wise get all the events.
    // and make sure that we update our stats accordingly.
    auto events = std::vector<event*>();
    if (0 != _monitor.get_events(events))
    {
      // then call the callback
      for ( auto& event : events )
      {
        // update the stats
        update_statistics(*event);

        // we are done with the event
        // so we can get rid of it.
        delete event;
      }
    }
  }

  /**
   * \brief get the events.
   * \param actualElapsedTimeMilliseconds the number of ms since the last time we published
   */
  void EventsPublisher::publish_statistics(const float actualElapsedTimeMilliseconds)
  {
    MYODDWEB_PROFILE_FUNCTION();
    try
    {
      _request.callback_statistics()(
        _id,
        actualElapsedTimeMilliseconds,
        _currentStatistics.numberOfEvents
        );

      // we are done with the stats
      _currentStatistics = { 0 };
    }
    catch (std::exception& e)
    {
      // the callback did something wrong!
      // log the error
      Logger::log(LogLevel::Error, L"Caught exception '%hs' in publish_statistics, check the callback!", e.what());
    }
  }

  /**
   * \brief update the stats with the given event
   * \paranm event the event we will update the stats with
   */
  void EventsPublisher::update_statistics(const event& event)
  {
    ++_currentStatistics.numberOfEvents;
  }


  /**
   * \brief publish all the events
   */
  void EventsPublisher::publish_events()
  {
    MYODDWEB_PROFILE_FUNCTION();

    // get the events.
    auto events = std::vector<event*>();
    if (0 == _monitor.get_events(events))
    {
      return;
    }

    // then call the callback
    for ( const auto& event : events )
    {
      try
      {
        // publish it
        _request.callback_events()(
          _id,
          event->IsFile,
          event->Name,
          event->OldName,
          event->Action,
          event->Error,
          event->TimeMillisecondsUtc
          );

        // update the stats
        update_statistics(*event);
      }
      catch (std::exception& e)
      {
        // the callback did something wrong!
        // log the error
        Logger::log(LogLevel::Error, L"Caught exception '%hs' in publish_events, check the callback!", e.what());
      }

      // we are done with the event
      // so we can get rid of it.
      delete event;
    }
  }
}
