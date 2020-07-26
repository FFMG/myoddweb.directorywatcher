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
  bool EventsPublisher::HasEventsElapsed(const float fElapsedTimeMilliseconds)
  {
    if( !_request.IsUsingEvents())
    {
      return false;
    }

    _elapsedEventsTimeMilliseconds += fElapsedTimeMilliseconds;
    if (_elapsedEventsTimeMilliseconds < static_cast<float>(_request.EventsCallbackRateMilliseconds()))
    {
      return false;
    }

    //  restart the timer.
    while (_elapsedEventsTimeMilliseconds > static_cast<float>(_request.EventsCallbackRateMilliseconds())) {
      _elapsedEventsTimeMilliseconds -= static_cast<float>(_request.EventsCallbackRateMilliseconds());
    }
    return true;
  }

  float EventsPublisher::HasStatisticsElapsed(const float fElapsedTimeMilliseconds)
  {
    // are we using stats?
    if( !_request.IsUsingStatistics())
    {
      return 0;
    }

    _elapsedStatisticsTimeMilliseconds += fElapsedTimeMilliseconds;
    if (_elapsedStatisticsTimeMilliseconds < static_cast<float>(_request.StatsCallbackRateMilliseconds()))
    {
      return 0;
    }

    const auto actualElapsedTimeMilliseconds = _elapsedStatisticsTimeMilliseconds;

    //  restart the timer.
    while (_elapsedStatisticsTimeMilliseconds > static_cast<float>(_request.StatsCallbackRateMilliseconds())) {
      _elapsedStatisticsTimeMilliseconds -= static_cast<float>(_request.StatsCallbackRateMilliseconds());
    }
    return actualElapsedTimeMilliseconds;
  }

  void EventsPublisher::Update(const float fElapsedTimeMilliseconds)
  {
    // first check the events
    UpdateEvents(fElapsedTimeMilliseconds);

    // then the stats
    UpdateStatistics(fElapsedTimeMilliseconds);
  }

  /**
   * \brief called at various intervals.
   * \param fElapsedTimeMilliseconds the number of ms since the last update
   */
  void EventsPublisher::UpdateEvents(float fElapsedTimeMilliseconds)
  {
    // check if we are ready.
    if (!HasEventsElapsed(fElapsedTimeMilliseconds))
    {
      return;
    }

    // get the events
    PublishEvents();
  }

  /**
   * \brief called at various intervals.
   * \param fElapsedTimeMilliseconds the number of ms since the last update
   */
  void EventsPublisher::UpdateStatistics(float fElapsedTimeMilliseconds)
  {
    // check if we are ready.
    const auto actualElapsedTimeMilliseconds = HasStatisticsElapsed(fElapsedTimeMilliseconds);
    if (actualElapsedTimeMilliseconds == 0 )
    {
      return;
    }

    // we need to double check that events are indeed supported
    // if not, then we need to do the updating ourselves.
    EnsureStatisticsAreUpToDateIfNotCollectingEvents();

    // then we can publish the stats
    PublishStatistics(actualElapsedTimeMilliseconds);
  }

  /**
   * \brief make sure that all the stats values are up to date
   *        if we are not collecting any information
   */
  void EventsPublisher::EnsureStatisticsAreUpToDateIfNotCollectingEvents()
  {
    // if we are collecting events, then things should be fine
    // we cannot get events for the purpose of updating stats
    // otherwiswe could loose events.
    if (_request.IsUsingEvents())
    {
      return;
    }

    // other wise get all the events.
    // and make sure that we update our stats accordingly.
    auto events = std::vector<Event*>();
    if (0 != _monitor.GetEvents(events))
    {
      // then call the callback
      for (auto it = events.begin(); it != events.end(); ++it)
      {
        const auto& event = (*it);

        // update the stats
        UpdateStatistics(*event);

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
  void EventsPublisher::PublishStatistics(const float actualElapsedTimeMilliseconds)
  {
    MYODDWEB_PROFILE_FUNCTION();
    try
    {
      _request.CallbackStatistics()(
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
      Logger::Log(LogLevel::Error, L"Caught exception '%hs' in PublishStatistics, check the callback!", e.what());
    }
  }

  /**
   * \brief update the stats with the given event
   * \paranm event the event we will update the stats with
   */
  void EventsPublisher::UpdateStatistics(const Event& event)
  {
    ++_currentStatistics.numberOfEvents;
  }


  /**
   * \brief publish all the events
   */
  void EventsPublisher::PublishEvents()
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
        // publish it
        _request.CallbackEvents()(
          _id,
          event->IsFile,
          event->Name,
          event->OldName,
          event->Action,
          event->Error,
          event->TimeMillisecondsUtc
          );

        // update the stats
        UpdateStatistics(*event);
      }
      catch (std::exception& e)
      {
        // the callback did something wrong!
        // log the error
        Logger::Log(LogLevel::Error, L"Caught exception '%hs' in PublishEvents, check the callback!", e.what());
      }

      // we are done with the event
      // so we can get rid of it.
      delete event;
    }
  }
}
