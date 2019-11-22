// Licensed to Florent Guelfucci under one or more agreements.
// Florent Guelfucci licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.
#include <Windows.h>
#include <utility>
#include "Monitor.h"
#include "../utils/Lock.h"
#include "../utils/Io.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    Monitor::Monitor(const __int64 id, const Request& request) :
      _id(id),
      _eventCollector(nullptr),
      _state(Stopped)
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
    __int64 Monitor::Id() const
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
      _eventCollector->AddRename(Path(), newFileName, oldFilename, isFile, EventError::None );
    }

    /**
     * \brief Add an error event to the list.
     * \param error the error we want to add.
     */
    void Monitor::AddEventError(const EventError error) const
    {
      _eventCollector->Add(EventAction::Unknown, Path(), L"", false, error );
    }

    /**
     * \brief fill the vector with all the values currently on record.
     * \param events the events we will be filling
     * \return the number of events we found.
     */
    long long Monitor::GetEvents(std::vector<Event>& events)
    {
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
    bool Monitor::Start()
    {
      if (Is(Started))
      {
        return true;
      }

      // guard for multiple entry.
      auto guard = Lock(_lock);

      // are we already started?
      if( Is(Started))
      {
        return true;
      }

      // we are starting
      _state = Starting;

      try
      {
        // derived class to start
        OnStart();

        // all good
        _state = Started;

        // done.
        return true;
      }
      catch (...)
      {
        _state = Stopped;
        AddEventError(EventError::CannotStart);
        return false;
      }
    }

    /**
     * \brief Stop the monitoring if needed.
     */
    void Monitor::Stop()
    {
      if (Is(Stopped))
      {
        return;
      }

      // guard for multiple entry.
      auto guard = Lock(_lock);

      // are we stopped already?
      if( Is(Stopped))
      {
        return;
      }

      // we are stopping
      _state = Stopping;

      try
      {
        // then do the stop
        OnStop();

        // we are now done
        _state = Stopped;
      }
      catch(... )
      {
        _state = Stopped;
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
