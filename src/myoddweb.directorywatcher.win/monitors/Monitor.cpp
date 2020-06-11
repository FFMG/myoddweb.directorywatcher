// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#include <Windows.h>
#include "Monitor.h"
#include "../utils/Io.h"
#include "../utils/Instrumentor.h"
#include "Base.h"

namespace myoddweb:: directorywatcher
{
  Monitor::Monitor( const __int64 id, threads::WorkerPool& workerPool, const Request& request) :
    Worker(),
    _id(id),
    _workerPool( workerPool ),
    _publisher(nullptr)
  {
    _request = new Request(request);
  }

  Monitor::~Monitor()
  {
    delete _request;
    _request = nullptr;

    delete _publisher;
    _publisher = nullptr;
  }

  /**
   * \return get the data collector
   */
  const Collector& Monitor::EventsCollector() const
  {
    return _eventCollector;
  }

  /**
   * Get the id of the monitor
   * @return __int64 the id
   */
  const long long& Monitor::Id() const
  {
    return _id;
  }

  /**
   * Get the current path.
   * @return the path being checked.
   */
  const wchar_t* Monitor::Path() const
  {
    return _request->Path();
  }

  /**
   * If this is a recursive monitor or not.
   * @return if recursive or not.
   */
  bool Monitor::Recursive() const
  {
    return _request->Recursive();
  }

  /**
   * \brief Add an event to our current log.
   * \param action the action that was performed, (added, deleted and so on)
   * \param fileName the name of the file/directory
   * \param isFile if it is a file or not
   */
  void Monitor::AddEvent(const EventAction action, const std::wstring& fileName, const bool isFile)
  {
    MYODDWEB_PROFILE_FUNCTION();
    _eventCollector.Add(action, Path(), fileName, isFile, EventError::None);
  }

  /**
   * \brief Add an event to our current log.
   * \param newFileName the new name of the file/directory
   * \param oldFilename the previous name
   * \param isFile if this is a file or not.
   */
  void Monitor::AddRenameEvent(const std::wstring& newFileName, const std::wstring& oldFilename, const bool isFile)
  {
    MYODDWEB_PROFILE_FUNCTION();
    _eventCollector.AddRename(Path(), newFileName, oldFilename, isFile, EventError::None );
  }

  /**
   * \brief add an event error to the queue
   * \param error the error event being added
   */
  void Monitor::AddEventError(const EventError error)
  {
    MYODDWEB_PROFILE_FUNCTION();
    _eventCollector.Add(EventAction::Unknown, Path(), L"", false, error );
  }

  /**
   * \brief fill the vector with all the values currently on record.
   * \param events the events we will be filling
   * \return the number of events we found.
   */
  long long Monitor::GetEvents(std::vector<Event*>& events)
  {
    MYODDWEB_PROFILE_FUNCTION();

    if (!Is(State::started))
    {
      return 0;
    }

    // get the events we collected.
    _eventCollector.GetEvents(events);

    // allow the base class to add/remove events.
    OnGetEvents(events);

    // then return how-ever many we found.  
    return static_cast<long long>(events.size());
  }

  /**
   * \brief Start the monitoring, if needed.
   * \return success or not.
   */
  bool Monitor::OnWorkerStart()
  {
    // are we already started?
    if( Is(State::started))
    {
      return true;
    }

    try
    {
      // start the callback after we started everything
      StartEventsPublisher();

      // done.
      return true;
    }
    catch (...)
    {
      AddEventError(EventError::CannotStart);
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
  bool Monitor::OnWorkerUpdate( const float fElapsedTimeMilliseconds)
  {
    if( _publisher != nullptr )
    {
      _publisher->Update(fElapsedTimeMilliseconds);
    }
    return !MustStop();
  }

  /**
   * \brief stop the worker
   */
  void Monitor::OnWorkerStop()
  {
    // nothing to do here
  }

  /**
   * \brief Stop the monitoring if needed.
   */
  void Monitor::OnWorkerEnd()
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
      AddEventError(EventError::CannotStop);
    }
  }

  /**
    * \brief Start the callback timer so we can publish events.
    */
  void Monitor::StartEventsPublisher()
  {
    MYODDWEB_PROFILE_FUNCTION();

    // whatever we are doing, we need to kill the current timer.
    delete _publisher;
    _publisher = nullptr;

    // create the new publisher.
    _publisher = new EventsPublisher( *this, ParentId(), *_request );
  }

  /**
   * \brief check if a given path is the same as the given one.
   * \param maybe the path we are checking against.
   * \return if the given path is the same as our path.
   */
  bool Monitor::IsPath(const std::wstring& maybe) const
  {
    return Io::AreSameFolders(maybe, _request->Path());
  }
}
