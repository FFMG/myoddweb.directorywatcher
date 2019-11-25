﻿// Licensed to Florent Guelfucci under one or more agreements.
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
      _callback(nullptr),
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
      return _request->Path;
    }

    /**
     * If this is a recursive monitor or not.
     * @return if recursive or not.
     */
    bool Monitor::Recursive() const
    {
      return _request->Recursive;
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
    long long Monitor::GetEvents(std::vector<Event>& events)
    {
      MYODDWEB_PROFILE_FUNCTION();

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
     * \return success or not.
     */
    bool Monitor::Start(EventCallback callback, long long callbackRateMs)
    {
      // set the callback in case we are updating it.
      // this uses the lock, so we should be fine.
      SetCallBack(callback, callbackRateMs );

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
     * \brief set the callback and how often we want to check for event, (and callback if we have any).
     * \param the callback we want to call
     * \param hw often we want to check for events.
     * \return
     */
    void Monitor::SetCallBack(EventCallback callback, const  long long callbackIntervalMs )
    {
      MYODDWEB_PROFILE_FUNCTION();

      // guard for multiple entry.
      auto guard = Lock(_lock);

      // null is allowed.
      if (_callback != callback)
      {
        _callback = callback;
      }

      if (_callbackTimer != nullptr)
      {
        _callbackTimer->Stop();
        delete _callbackTimer;
        _callbackTimer = nullptr;
      }

      // zero are allowed.
      if (callbackIntervalMs != 0)
      {
        _callbackTimer = new Timer();
        _callbackTimer->Start([&]() {
          PublishEvents();
          }, callbackIntervalMs);
      }
    }

    /**
     * \brief get all the events and send them over to the callback.
     */
    void Monitor::PublishEvents()
    {
      MYODDWEB_PROFILE_FUNCTION();

      // guard for multiple entry.
      auto guard = Lock(_lock);

      auto events = std::vector<Event>();
      if (0 == GetEvents(events))
      {
        return;
      }

      for (auto it = events.begin(); it != events.end(); ++it)
      {
        _callback(
          Id(),
          (*it).IsFile,
          (*it).Name,
          (*it).OldName,
          (*it).Action,
          (*it).Error,
          (*it).TimeMillisecondsUtc
          );
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
      return Io::AreSameFolders(maybe, _request->Path);
    }
  }
}
