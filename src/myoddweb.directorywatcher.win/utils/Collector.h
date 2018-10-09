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
#pragma once
#include <string>
#include <vector>
#include <mutex>
#include "EventAction.h"
#include "EventInformation.h"
#include "Event.h"

namespace myoddweb
{
  namespace directorywatcher
  {
    /**
     * \brief Class that contains and manages all the events.
     */
    class Collector
    {
    public:
      Collector();
      virtual ~Collector();

    private:
      explicit Collector( short maxAgeMs );

    public:
      void Add(ManagedEventAction action, const std::wstring& path, const std::wstring& filename, bool isFile, ManagedEventError error);
      void AddRename(const std::wstring& path, const std::wstring&newFilename, const std::wstring&oldFilename, bool isFile, ManagedEventError error);

      /**
       * \brief fill the vector with all the values currently on record.
       * \param events the events we will be filling
       * \return the number of events we found.
       */
      long long GetEvents( std::vector<Event>& events);

    private:
      void Add(ManagedEventAction action, const std::wstring& path, const std::wstring& filename, const std::wstring& oldFileName, bool isFile, ManagedEventError error);

      /**
       * \brief This is the oldest number of ms we want something to be.
       * It is *only* removed if _maxInternalCounter is reached.
       */
      const short _maxCleanupAgeMillisecons;

      /**
       * The next time we want to check for cleanup
       */
      long long _nextCleanupTimeCheck = 0;

      /**
       * \brief cleanup the vector if our internal counter has being reached.
       */
      void CleanupEvents();

      /**
       * \brief Add an event to the vector and remove older events.
       * \param event
       */
      void AddEventInformation(const EventInformation& event);

      /**
       * \brief the locks so we can add data.
       */
      std::recursive_mutex _lock;

      /**
       * \brief the events list
       */
      typedef std::vector<EventInformation> Events;
      Events _events;

      /**
       * \brief Get the time now in milliseconds since 1970
       * \return the current ms time
       */
      static long long GetMillisecondsNowUtc();

      /**
       * \brief convert an EventAction to an un-managed IAction
       * so it can be returned to the calling interface.
       * Our EventAction are fairly similar to the Managed IAction, but not all values are the same
       * For example, RenamedOld and RenamedNew are just 'ManagedAction::Renamed'
       */
      static int ConvertEventActionToUnManagedAction(const ManagedEventAction& action);

      /**
       * \brief check if the given information already exists in the source
       * \param source the collection of events we will be looking in
       * \param duplicate the event information we want to add.
       * \return if the event information is already in the 'source'
       */
      static bool IsOlderDuplicate(const std::vector<Event>& source, const Event& duplicate);

      /**
       * \brief go around all the renamed events and look the the ones that are 'invalid'
       * The ones that do not have a new/old name.
       * \param source the collection of events we will be looking in
       */
      static void ValidateRenames(std::vector<Event>& source );

      /**
       * \brief copy the current content of the events into a local variable.
       * Then erase the current content so we can continue receiving data.
       */
      void CloneEventsAndEraseCurrent(Events& clone);
    };
  }
}