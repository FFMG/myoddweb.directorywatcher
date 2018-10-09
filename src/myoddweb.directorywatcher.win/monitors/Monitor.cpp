﻿//This file is part of Myoddweb.Directorywatcher.
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
#include "Monitor.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    Monitor::Monitor(__int64 id, const Request& request) :
      _id(id),
      _request(request),
      _eventCollector(nullptr)
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
    void Monitor::AddEvent(const ManagedEventAction action, const std::wstring& fileName, bool isFile) const
    {
      _eventCollector->Add(action, Path(), fileName, isFile, ManagedEventError::None);
    }

    /**
     * \brief Add an event to our current log.
     * \param newFileName
     * \param oldFilename
     * \param isFile
     */
    void Monitor::AddRenameEvent(const std::wstring& newFileName, const std::wstring& oldFilename, bool isFile) const
    {
      _eventCollector->AddRename(Path(), newFileName, oldFilename, isFile, ManagedEventError::None );
    }

    /**
     * \brief Add an error event to the list.
     * \param error the error we want to add.
     */
    void Monitor::AddEventError(const ManagedEventError error) const
    {
      _eventCollector->Add(ManagedEventAction::Unknown, L"", L"", false, error );
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
  }
}
