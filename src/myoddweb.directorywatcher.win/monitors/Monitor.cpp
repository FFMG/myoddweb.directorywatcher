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
  Monitor::Monitor(const __int64 id, threads::WorkerPool& workerPool, const Request& request) :
    _id(id),
    _workerPool( workerPool ),
    _eventCollector(nullptr),
    _callbackTimer(nullptr)
  {
    _eventCollector = new Collector();
    _request = new Request(request);
  }

  Monitor::~Monitor()
  {
    delete _request;
    _request = nullptr;

    delete _eventCollector;
    _eventCollector = nullptr;

    delete _callbackTimer;
    _callbackTimer = nullptr;
  }

  /**
   * \return get the data collector
   */
  Collector& Monitor::EventsCollector() const
  {
    return *_eventCollector;
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
   * \param action
   * \param fileName
   * \param isFile
   */
  void Monitor::AddEvent(const EventAction action, const std::wstring& fileName, const bool isFile) const
  {
    MYODDWEB_PROFILE_FUNCTION();
    _eventCollector->Add(action, Path(), fileName, isFile, EventError::None);
  }

  /**
   * \brief Add an event to our current log.
   * \param newFileName
   * \param oldFilename
   * \param isFile
   */
  void Monitor::AddRenameEvent(const std::wstring& newFileName, const std::wstring& oldFilename, const bool isFile) const
  {
    MYODDWEB_PROFILE_FUNCTION();
    _eventCollector->AddRename(Path(), newFileName, oldFilename, isFile, EventError::None );
  }

  /**
   * \brief Add an error event to the list.
   * \param error the error we want to add.
   */
  void Monitor::AddEventError(const EventError error) const
  {
    MYODDWEB_PROFILE_FUNCTION();
    _eventCollector->Add(EventAction::Unknown, Path(), L"", false, error );
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
    _eventCollector->GetEvents(events);

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
      StartCallBack();

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
    return !MustStop();
  }

  /**
   * \brief stop the worker
   */
  void Monitor::OnStop()
  {
    // we can now stop us.
    Worker::OnStop();

    // stop the callback
    StopCallBack();
  }

  /**
   * \brief Stop the monitoring if needed.
   */
  void Monitor::OnWorkerEnd()
  {
    MYODDWEB_PROFILE_FUNCTION();

    try
    {
      // wait for the timer to end ... then close it.
      while (threads::WaitResult::complete != WorkerPool().WaitFor(*_callbackTimer, MYODDWEB_WAITFOR_WORKER_COMPLETION))
      {
        MYODDWEB_OUT("Timeout waiting to complete callback timer!\n");
      }
      delete _callbackTimer;
      _callbackTimer = nullptr;
    }
    catch (...)
    {
      AddEventError(EventError::CannotStop);
    }
  }

  /**
   * \brief Stop the callback timer to we can stop publishing.
   */
  void Monitor::StopCallBack() const
  {
    MYODDWEB_PROFILE_FUNCTION();

    // do we have a callback to stop?
    if (_callbackTimer == nullptr)
    {
      return;
    }

    // stop the call back worker.
    WorkerPool().Stop(*_callbackTimer);
  }

  /**
    * \brief Start the callback timer so we can publish events.
    */
  void Monitor::StartCallBack()
  {
    MYODDWEB_PROFILE_FUNCTION();

    // whatever we are doing, we need to kill the current timer.
    StopCallBack();

    // null is allowed
    if (nullptr == _request->Callback())
    {
      return;
    }

    // zero are allowed.
    if (0 == _request->CallbackRateMs())
    {
      return;
    }

    _callbackTimer = new Timer( std::bind( &Monitor::PublishEvents, this),_request->CallbackRateMs());
    WorkerPool().Add(*_callbackTimer);
  }

  /**
   * \brief get all the events and send them over to the callback.
   */
  void Monitor::PublishEvents()
  {
    MYODDWEB_PROFILE_FUNCTION();

    // check that we are ready
    if (!Is(State::started))
    {
      return;
    }

    // get the events.
    auto events = std::vector<Event*>();
    if (0 == GetEvents(events))
    {
      return;
    }

    // then call the callback
    for (auto it = events.begin(); it != events.end(); ++it)
    {
      const auto& event = (*it);
      try
      {
        if (nullptr != _request->Callback())
        {
          _request->Callback()(
            ParentId(),
            event->IsFile,
            event->Name,
            event->OldName,
            event->Action,
            event->Error,
            event->TimeMillisecondsUtc
          );
        }
      }
      catch( ... )
      {
        // the callback did something wrong!
        // we should log it somewhere.
      }

      // we are done with the event
      // so we can get rid of it.
      delete event;
    }
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
