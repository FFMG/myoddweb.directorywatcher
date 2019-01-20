//This file is part of Myoddweb.Directorywatcher.
//
//    Myoddweb.Directorywatcher is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    Myoddweb.Directorywatcher is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Myoddweb.Directorywatcher.  If not, see<https://www.gnu.org/licenses/gpl-3.0.en.html>.
#include <Windows.h>
#include <utility>
#include "Monitor.h"
#include "../utils/Lock.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    Monitor::Monitor(const __int64 id, Request request) :
      _id(id),
      _request(std::move(request)),
      _eventCollector(nullptr),
      _state(Stopped)
    {
      _eventCollector = new Collector();
    }

    Monitor::~Monitor()
    {
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
    const std::wstring& Monitor::Path() const
    {
      return _request.Path;
    }

    /**
     * If this is a recursive monitor or not.
     * @return if recursive or not.
     */
    bool Monitor::Recursive() const
    {
      return _request.Recursive;
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
    long long Monitor::GetEvents(std::vector<Event>& events) const
    {
      return _eventCollector->GetEvents(events);
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
      catch(std::exception const & ex )
      {
        _state = Stopped;
        AddEventError(EventError::CannotStop);
      }
    }
  }
}
