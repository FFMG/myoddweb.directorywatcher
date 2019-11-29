// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#include <Windows.h>
#include <utility>
#include "Monitor.h"
#include "../utils/Lock.h"
#include "../utils/Io.h"
#include "../utils/Instrumentor.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    Monitor::Monitor(const __int64 id, const Request& request) :
      _id(id),
      _eventCollector(nullptr),
      _callbackTimer(nullptr),
      _state(State::Stopped)
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

      if (!Is(State::Started))
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
     * \brief return if the current state is the same as the one we are after.
     * \param state the state we are checking against.
     */
    bool Monitor::Is(const State state) const
    {
      return _state == state;
    }

    /**
     * \brief Start the monitoring, if needed.
     * \param callback
     * \param callbackRateMs how often we want to callback
     * \return success or not.
     */
    bool Monitor::Start()
    {
      if (Is(State::Started))
      {
        return true;
      }

      // guard for multiple entry.
      auto guard = Lock(_lock);

      // are we already started?
      if( Is(State::Started))
      {
        return true;
      }

      // we are starting
      _state = State::Starting;

      try
      {
        // start the callback
        StartCallBack();

        // derived class to start
        OnStart();

        // all good
        _state = State::Started;

        // done.
        return true;
      }
      catch (...)
      {
        _state = State::Stopped;
        AddEventError(EventError::CannotStart);
        return false;
      }
    }

    /**
     * \brief Stop the callback timer to we can stop publishing.
     */
    void Monitor::StopCallBack()
    {
      MYODDWEB_PROFILE_FUNCTION();

      // guard for multiple entry.
      auto guard = Lock(_lock);

      // do we have a callback to stop?
      if (_callbackTimer == nullptr)
      {
        return;
      }

      _callbackTimer->Stop();
      delete _callbackTimer;
      _callbackTimer = nullptr;
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

      _callbackTimer = new Timer();
      _callbackTimer->Start([&]() 
        {
          PublishEvents();
        }, 
        _request->CallbackRateMs()
      );
    }

    /**
     * \brief get all the events and send them over to the callback.
     */
    void Monitor::PublishEvents()
    {
      MYODDWEB_PROFILE_FUNCTION();

      // guard for multiple entry.
      auto guard = Lock(_lock);

      // check that we are ready
      if (!Is(State::Started))
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
              Id(),
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
     * \brief Stop the monitoring if needed.
     */
    void Monitor::Stop()
    {
      MYODDWEB_PROFILE_FUNCTION();

      if (Is(State::Stopped))
      {
        return;
      }

      // guard for multiple entry.
      auto guard = Lock(_lock);

      // are we stopped already?
      if( Is(State::Stopped))
      {
        return;
      }

      // we are stopping
      _state = State::Stopping;

      try
      {
        // we have to kill the callback timer
        StopCallBack();

        // then do the stop
        OnStop();

        // we are now done
        _state = State::Stopped;
      }
      catch(... )
      {
        _state = State::Stopped;
        AddEventError(EventError::CannotStop);
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
}
