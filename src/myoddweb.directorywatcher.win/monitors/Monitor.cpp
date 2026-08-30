// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#include <Windows.h>
#include "Monitor.h"
#include "../utils/Io.h"
#include "../utils/Instrumentor.h"
#include "../utils/Logger.h"
#include "../utils/LogLevel.h"

namespace myoddweb:: directorywatcher
{
  Monitor::Monitor( const long long id, threads::WorkerPool& workerPool, const Request& request) :
    Worker( id ),
    _workerPool( workerPool ),
    _request( request ),
                      // we will keep data for as long as we need it, either the event time if not zero, (as it updates the stats)
                      // otherwise we will set the time to the stats time
                      // if both of them are zero then nothing will be collected
    _eventCollector(request.events_callback_rate_milliseconds() == 0 ? request.stats_callback_rate_milliseconds() : request.events_callback_rate_milliseconds()),
    _publisher(nullptr)
  {
  }

  Monitor::~Monitor()
  {
    if( !completed() )
    {
      // log the error
      Logger::log(id(), LogLevel::Error, L"Trying to dispose of a monitor that was never completed" );
    }
    delete _publisher;
    _publisher = nullptr;
  }

  /**
   * \return get the data collector
   */
  const Collector& Monitor::events_collector() const
  {
    return _eventCollector;
  }

  /**
   * \brief the patht that is being monitored.
   */
  const wchar_t* Monitor::path() const
  {
    return _request.path();
  }

  /**
   * \brief If we are recursively checking this folder or not.
   */
  bool Monitor::recursive() const
  {
    return _request.recursive();
  }

  /**
   * \brief Add an event to our current log.
   * \param action the action that was performed, (added, deleted and so on)
   * \param fileName the name of the file/directory
   * \param isFile if it is a file or not
   */
  void Monitor::add_event(const EventAction action, const std::wstring& fileName, const bool isFile)
  {
    MYODDWEB_PROFILE_FUNCTION();
    _eventCollector.add(action, path(), fileName, isFile, EventError::None);
  }

  /**
   * \brief Add an event to our current log.
   * \param newFileName the new name of the file/directory
   * \param oldFilename the previous name
   * \param isFile if this is a file or not.
   */
  void Monitor::add_rename_event(const std::wstring& newFileName, const std::wstring& oldFilename, const bool isFile)
  {
    MYODDWEB_PROFILE_FUNCTION();
    _eventCollector.add_rename(path(), newFileName, oldFilename, isFile, EventError::None );
  }

  /**
   * \brief add an event error to the queue
   * \param error the error event being added
   */
  void Monitor::add_event_error(const EventError error)
  {
    MYODDWEB_PROFILE_FUNCTION();
    _eventCollector.add(EventAction::Unknown, path(), L"", false, error );
  }

  /**
   * \brief fill the vector with all the values currently on record.
   * \param events the events we will be filling
   * \return the number of events we found.
   */
  long long Monitor::get_events(std::vector<event*>& events)
  {
    MYODDWEB_PROFILE_FUNCTION();

    if (!is(State::started))
    {
      return 0;
    }

    // get the events we collected.
    _eventCollector.get_events(events);

    // allow the base class to add/remove events.
    on_get_events(events);

    // then return how-ever many we found.
    return static_cast<long long>(events.size());
  }

  /**
   * \brief Start the monitoring, if needed.
   * \return success or not.
   */
  bool Monitor::on_worker_start()
  {
    // are we already started?
    if( is(State::started))
    {
      return true;
    }

    try
    {
      // start the callback after we started everything
      start_events_publisher();

      // done.
      return true;
    }
    catch (...)
    {
      add_event_error(EventError::CannotStart);

      save_current_exception();

      return false;
    }
  }

  /**
   * \brief Give the worker a chance to do something in the loop
   *        Workers can do _all_ the work at once and simply return false
   *        or if they have a tight look they can return true until they need to come out.
   * \param fElapsedTimeMilliseconds the amount of time since the last time we made this call.
   * \return true if we want to continue or false if we want to end the thread
   */
  bool Monitor::on_worker_update( const float fElapsedTimeMilliseconds)
  {
    if( _publisher != nullptr )
    {
      _publisher->update(fElapsedTimeMilliseconds);
    }
    return !must_stop();
  }

  /**
   * \brief stop the worker
   */
  void Monitor::on_worker_stop()
  {
    // nothing to do here
  }

  /**
   * \brief Stop the monitoring if needed.
   */
  void Monitor::on_worker_end()
  {
    MYODDWEB_PROFILE_FUNCTION();
    try
    {
      // clean the publisher
      delete _publisher;
      _publisher = nullptr;
    }
    catch (...)
    {
      add_event_error(EventError::CannotStop);

      save_current_exception();
    }
  }

  /**
    * \brief Start the callback timer so we can publish events.
    */
  void Monitor::start_events_publisher()
  {
    MYODDWEB_PROFILE_FUNCTION();

    // whatever we are doing, we need to kill the current timer.
    delete _publisher;
    _publisher = nullptr;

    // create the new publisher.
    _publisher = new EventsPublisher( *this, parent_id(), _request );
  }

  /**
   * \brief check if a given path is the same as the given one.
   * \param maybe the path we are checking against.
   * \return if the given path is the same as our path.
   */
  bool Monitor::is_path(const std::wstring& maybe) const
  {
    return Io::are_same_folders(maybe, _request.path());
  }
}
